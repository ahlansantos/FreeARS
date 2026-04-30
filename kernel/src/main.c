#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "graphics/font.h"
#include "drivers/keyboard.h"

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static struct limine_framebuffer *fbi;
static uint32_t pw;
static uint32_t gx=8, gy=8;
static uint32_t fg=0xFFFFFF, bg=0x00111122;
static uint64_t total_ram=0;
static uint64_t tsc_hz=0;
static uint64_t boot_tsc=0;

#define HEAP_BASE 0x1000000 
static uint64_t heap_current = HEAP_BASE;

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~7; 
    void* ptr = (void*)heap_current;
    heap_current += size;
    if (heap_current > HEAP_BASE + 0x4000000) return NULL; 
    return ptr;
}
void kfree(void* ptr) { (void)ptr; }

static inline void outb(uint16_t p, uint8_t v){ asm volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t v; asm volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }

static inline uint64_t rdtsc(void){
    uint32_t lo, hi;
    asm volatile("rdtsc":"=a"(lo),"=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint16_t pit_read(void){
    outb(0x43, 0x00);
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40);
    return ((uint16_t)hi << 8) | lo;
}

static void tsc_calibrate(void){
    outb(0x43, 0x34);
    outb(0x40, 0xFF);
    outb(0x40, 0xFF);

    while(pit_read() < 60000) asm volatile("pause");
    while(pit_read() < 60000) asm volatile("pause");

    uint16_t start = pit_read();
    uint64_t tsc_start = rdtsc();

    uint16_t target = start - 10000;
    while(pit_read() > target) asm volatile("pause");

    uint64_t tsc_end = rdtsc();
    uint64_t cycles = tsc_end - tsc_start;
    
    tsc_hz = (cycles * 1193182ULL) / 10000ULL;

    if(tsc_hz < 100000000ULL) tsc_hz = 1000000000ULL;
}

static uint64_t uptime_ms(void){
    if (tsc_hz == 0) return 0;
    uint64_t cycles = rdtsc() - boot_tsc;
    return (cycles * 1000ULL) / tsc_hz;
}

static uint32_t get_ticks(void){
    return (uint32_t)(uptime_ms() / 10);
}

static void sleep_ms(uint32_t ms){
    uint64_t start = rdtsc();
    uint64_t cycles_needed = (tsc_hz * (uint64_t)ms) / 1000ULL;
    uint64_t end_tsc = start + cycles_needed;
    
    while(rdtsc() < end_tsc) {
        asm volatile("pause");
    }
}

typedef struct { uint16_t limit; uint64_t base; } __attribute__((packed)) idt_ptr_t;
typedef struct {
    uint16_t base_low; uint16_t selector; uint8_t ist; uint8_t flags;
    uint16_t base_mid; uint32_t base_high; uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

static void idt_set_gate(uint8_t n, uint64_t h, uint16_t s, uint8_t f){
    idt[n].base_low=h&0xFFFF; idt[n].base_mid=(h>>16)&0xFFFF; idt[n].base_high=(h>>32)&0xFFFFFFFF;
    idt[n].selector=s; idt[n].ist=0; idt[n].flags=f; idt[n].reserved=0;
}

static void px(uint32_t x, uint32_t y, uint32_t c){
    if(x<fbi->width && y<fbi->height) ((volatile uint32_t*)fbi->address)[y*pw+x]=c;
}
static void chr(uint32_t x, uint32_t y, char c, uint32_t cf, uint32_t cb){
    const uint8_t *g=font[(unsigned char)c];
    for(int r=0;r<16;r++) for(int co=0;co<8;co++) px(x+co,y+r,(g[r]&(1<<(7-co)))?cf:cb);
}
static void scroll(void){
    volatile uint32_t *fbb=fbi->address;
    for(uint32_t r=0;r<fbi->height-16;r++){
        uint32_t *d=(uint32_t*)(fbb+r*pw), *s=(uint32_t*)(fbb+(r+16)*pw);
        for(uint32_t c=0;c<fbi->width;c++) d[c]=s[c];
    }
    for(uint32_t r=fbi->height-16;r<fbi->height;r++){
        uint32_t *p=(uint32_t*)(fbb+r*pw);
        for(uint32_t c=0;c<fbi->width;c++) p[c]=bg;
    }
    gy=fbi->height-16;
}
static void put(char c){
    if(c=='\n'){gx=8;gy+=16;if(gy>=fbi->height-16)scroll();return;}
    if(c=='\b'){if(gx>=16){gx-=8;chr(gx,gy,' ',fg,bg);}return;}
    chr(gx,gy,c,fg,bg);gx+=8;
    if(gx>=fbi->width-8){gx=8;gy+=16;if(gy>=fbi->height-16)scroll();}
}
static void print(const char *s){ for(int i=0;s[i];i++) put(s[i]); }
static void println(const char *s){ print(s); put('\n'); }
static void print_int(uint32_t n){
    char b[11]; int i=10; b[i--]=0;
    do{ b[i--]='0'+n%10; n/=10; }while(n);
    print(&b[i+1]);
}
static void print_hex(uint32_t n){
    char h[]="0123456789ABCDEF"; print("0x");
    for(int i=7;i>=0;i--) put(h[(n>>(i*4))&0xF]);
}
static void clear(void){
    for(uint32_t y=0;y<fbi->height;y++)
        for(uint32_t x=0;x<fbi->width;x++) px(x,y,bg);
    gx=8; gy=8;
}

static int strcmp(const char *a,const char *b){ while(*a&&*a==*b){a++;b++;} return *a-*b; }
static int startswith(const char *s,const char *p){ while(*p) if(*s++!=*p++) return 0; return 1; }

void exception_handler(void){
    clear(); fg=0xFF0000;
    println("\n=== SYSTEM EXCEPTION ===");
    println("System halted - fatal error");
    asm volatile("cli; 1: hlt; jmp 1b");
}

__attribute__((used,naked)) static void isr_common(void){
    asm volatile(
        "pushq %%rax; pushq %%rbx; pushq %%rcx; pushq %%rdx;"
        "pushq %%rsi; pushq %%rdi; pushq %%rbp;"
        "pushq %%r8;  pushq %%r9;  pushq %%r10; pushq %%r11;"
        "pushq %%r12; pushq %%r13; pushq %%r14; pushq %%r15;"
        "cld; call exception_handler;"
        "popq %%r15; popq %%r14; popq %%r13; popq %%r12;"
        "popq %%r11; popq %%r10; popq %%r9;  popq %%r8;"
        "popq %%rbp; popq %%rdi; popq %%rsi;"
        "popq %%rdx; popq %%rcx; popq %%rbx; popq %%rax;"
        "addq $16, %%rsp; iretq" ::: "memory"
    );
}

__attribute__((used,naked)) static void isr0(void)  { asm volatile("pushq $0; pushq $0;  jmp isr_common"); }
__attribute__((used,naked)) static void isr6(void)  { asm volatile("pushq $0; pushq $6;  jmp isr_common"); }
__attribute__((used,naked)) static void isr8(void)  { asm volatile("pushq $0; pushq $8;  jmp isr_common"); }
__attribute__((used,naked)) static void isr13(void) { asm volatile("pushq $0; pushq $13; jmp isr_common"); }
__attribute__((used,naked)) static void isr14(void) { asm volatile("pushq $0; pushq $14; jmp isr_common"); }

static void idt_init(void){
    for(int i=0;i<256;i++) idt_set_gate(i,(uint64_t)isr_common,0x08,0x8E);
    idt_set_gate(0, (uint64_t)isr0, 0x08,0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x08,0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x08,0x8E);
    idt_set_gate(13,(uint64_t)isr13,0x08,0x8E);
    idt_set_gate(14,(uint64_t)isr14,0x08,0x8E);
    idt_ptr.limit=sizeof(idt_entry_t)*256-1;
    idt_ptr.base=(uint64_t)&idt;
    asm volatile("lidt %0"::"m"(idt_ptr));
}

static void cmd_help(void){
    fg=0x00FFFF; println("\n  === Commands ==="); fg=0xFFFFFF;
    println("  help / clear / uname / echo / sleep / ticks / crash / fastfetch / reboot / memtest");
    println("");
}

static void cmd_memtest(void) {
    fg = 0xDDDDDD;
    print("  Allocating 1KB... ");
    void* ptr = kmalloc(1024);
    if(ptr) {
        fg = 0x00FF00;
        println("OK");
        print("  Address: 0x"); print_hex((uint32_t)(uint64_t)ptr);
        println("");
    } else {
        fg = 0xFF0000;
        println("FAILED");
    }
}

static void cmd_fastfetch(void){
    clear(); fg=0x88AACC;
    println("   ______                        _____   _____ ");
    println("  |  ____|                 /\\   |  __ \\ / ____|");
    println("  | |__ _ __ ___  ___     /  \\  | |__) | (___  ");
    fg=0xFFFFFF;
    println("  |  __| '__/ _ \\/ _ \\   / /\\ \\ |  _  / \\___ \\ ");
    println("  | |  | | |  __/  __/  / ____ \\| | \\ \\ ____) |");
    fg=0x88AACC;
    println("  |_|  |_|  \\___|\\___| /_/    \\_\\_|  \\_\\_____/ ");
    println("");
    fg=0x88CC88; println("  user@FreeARS"); fg=0xAAAAAA; println("  -----------");
    fg=0xDDDDDD; print("  OS:       "); fg=0x88CC88; println("FreeARS 0.04");
    fg=0xDDDDDD; print("  Kernel:   "); fg=0x88CC88; println("x86_64 Limine UEFI");
    fg=0xDDDDDD; print("  Shell:    "); fg=0x88CC88; println("fsh 0.4");
    uint64_t ms=uptime_ms();
    uint32_t s=(uint32_t)(ms/1000), h=s/3600, m=(s%3600)/60; s=s%60;
    fg=0xDDDDDD; print("  Uptime:   "); fg=0x88CC88;
    if(h){print_int(h);print("h ");} if(m){print_int(m);print("m ");} print_int(s); println("s");
    fg=0xDDDDDD; print("  RAM:      "); fg=0x88CC88;
    if(total_ram>=1073741824){print_int(total_ram/1073741824);println(" GB");}
    else{print_int(total_ram/1048576);println(" MB");}
    fg=0xDDDDDD; print("  Display:  "); fg=0x88CC88;
    print_int(fbi->width); print("x"); print_int(fbi->height);
    print(" @ "); print_int(fbi->bpp); println("bpp");
    fg=0xDDDDDD; print("  CPU:      "); fg=0x88CC88;
    uint32_t eax,ebx,ecx,edx; char cpu[49]={0};
    asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x80000002));
    *(uint32_t*)(cpu+ 0)=eax;*(uint32_t*)(cpu+ 4)=ebx;*(uint32_t*)(cpu+ 8)=ecx;*(uint32_t*)(cpu+12)=edx;
    asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x80000003));
    *(uint32_t*)(cpu+16)=eax;*(uint32_t*)(cpu+20)=ebx;*(uint32_t*)(cpu+24)=ecx;*(uint32_t*)(cpu+28)=edx;
    asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(0x80000004));
    *(uint32_t*)(cpu+32)=eax;*(uint32_t*)(cpu+36)=ebx;*(uint32_t*)(cpu+40)=ecx;*(uint32_t*)(cpu+44)=edx;
    cpu[48]=0; int cp=0; while(cpu[cp]==' ') cp++; println(&cpu[cp]);
    fg=0xDDDDDD; print("  TSC:      "); fg=0x88CC88;
    print_int((uint32_t)(tsc_hz/1000000)); println(" MHz");
    println("");
    uint32_t cols[]={0xCC88AA,0xCCCC88,0x88CC88,0x88AACC,0x8888CC};
    for(int r=0;r<5;r++){print("  ");fg=cols[r];for(int c=0;c<30;c++)print("#");println("");}
    println("");
}

static void shell(void){
    char in[256];
    while(1){
        fg=0x00FF00; print("freeARS> "); fg=0xFFFFFF;
        keyboard_readline(in, 256);
        if(!strcmp(in,"help"))        cmd_help();
        else if(!strcmp(in,"clear"))  clear();
        else if(!strcmp(in,"uname")){ fg=0x00FF00; println("  FreeARS 0.04 - Limine x86_64"); }
        else if(startswith(in,"echo "))  { fg=0x00FF00; print("  "); println(in+5); }
        else if(!strcmp(in,"ticks")){
            fg=0x00FF00;
            print("  Ticks: ");   print_int(get_ticks());
            print("  Uptime: ");  print_int((uint32_t)(uptime_ms()/1000)); println("s");
        }
        else if(startswith(in,"sleep ")){
            unsigned long long ms=0;
            int valid = 0;
            for(int i=6; in[i]>='0' && in[i]<='9'; i++) {
                ms = ms*10 + (in[i]-'0');
                valid = 1;
            }
            
            if(!valid || ms == 0) {
                fg=0xFF0000; println("  Invalid number");
            } else {
                if(ms > 3600000) ms = 3600000; 
                
                fg=0x00FF00; print("  Sleeping "); print_int((uint32_t)ms); println("ms...");
                sleep_ms((uint32_t)ms);
                fg=0x00FF00; println("  Done!");
            }
        }
        else if(!strcmp(in,"crash")){
            fg=0xFF0000; println("  Crashing...");
            volatile int a=10,b=0,c=a/b; (void)c;
        }
        else if(!strcmp(in,"fastfetch")) cmd_fastfetch();
        else if(!strcmp(in,"memtest"))   cmd_memtest();
        else if(!strcmp(in,"reboot"))    outb(0x64,0xFE);
        else if(in[0]){ fg=0xFF0000; print("  not found: "); println(in); }
    }
}

static void hcf(void){ for(;;) asm("hlt"); }

void terminal_putchar(char c) {
    put(c);
}

void kmain(void){
    if(LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)==false) hcf();
    if(framebuffer_request.response==NULL||framebuffer_request.response->framebuffer_count<1) hcf();
    fbi=framebuffer_request.response->framebuffers[0];
    pw=fbi->pitch/4;
    if(memmap_request.response!=NULL)
        for(uint64_t i=0;i<memmap_request.response->entry_count;i++){
            struct limine_memmap_entry *e=memmap_request.response->entries[i];
            if(e->type==LIMINE_MEMMAP_USABLE) total_ram+=e->length;
        }
    
    tsc_calibrate();
    boot_tsc=rdtsc();
    
    idt_init();
    asm volatile("sti");
    
    clear();
    fg=0x88AACC;
    println("   ______                        _____   _____ ");
    println("  |  ____|                 /\\   |  __ \\ / ____|");
    println("  | |__ _ __ ___  ___     /  \\  | |__) | (___  ");
    fg=0xFFFFFF;
    println("  |  __| '__/ _ \\/ _ \\   / /\\ \\ |  _  / \\___ \\ ");
    println("  | |  | | |  __/  __/  / ____ \\| | \\ \\ ____) |");
    fg=0x88AACC;
    println("  |_|  |_|  \\___|\\___| /_/    \\_\\_|  \\_\\_____/ ");
    println("");
    fg=0x88CC88; println("  FreeARS 0.04"); println("");
    fg=0xDDDDDD; print("  Framebuffer: "); fg=0x88CC88;
    print_int(fbi->width); print("x"); print_int(fbi->height); println("");
    fg=0xDDDDDD; print("  RAM: "); fg=0x88CC88;
    if(total_ram>=1073741824){print_int(total_ram/1073741824);println(" GB");}
    else{print_int(total_ram/1048576);println(" MB");}
    fg=0xDDDDDD; print("  TSC: "); fg=0x88CC88;
    print_int((uint32_t)(tsc_hz/1000000)); println(" MHz");
    println("");
    fg=0xAAAAAA; println("  Type 'help' for available commands."); println("");
    shell();
    hcf();
}