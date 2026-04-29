#include "../graphics/fb.h"
#include "../fs/fs.h"
#include "../mm/kmalloc.h"
#include "../mm/paging.h"
#include "../sys/idt.h"
#include "../drivers/io.h"
#include "../drivers/pic.h"
#include "../drivers/timer.h"
#include <stdint.h>

static uint32_t gfx_x=8,gfx_y=8,gfx_fg=0xFFFFFF,gfx_bg=0x000000;

void gfx_scroll(){
    for(uint32_t r=0;r<fb.height-16;r++){
        uint32_t*d=(uint32_t*)((uint8_t*)fb.address+r*fb.pitch);
        uint32_t*s=(uint32_t*)((uint8_t*)fb.address+(r+16)*fb.pitch);
        for(uint32_t c=0;c<fb.width;c++)d[c]=s[c];
    }
    for(uint32_t r=fb.height-16;r<fb.height;r++){
        uint32_t*p=(uint32_t*)((uint8_t*)fb.address+r*fb.pitch);
        for(uint32_t c=0;c<fb.width;c++)p[c]=gfx_bg;
    }
    gfx_y=fb.height-16;
}

void gfx_putchar(char c){
    if(c=='\n'){gfx_x=8;gfx_y+=16;if(gfx_y>=fb.height-16)gfx_scroll();return;}
    if(c=='\b'){if(gfx_x>=16){gfx_x-=8;fb_draw_char(gfx_x,gfx_y,' ',gfx_fg,gfx_bg);}return;}
    fb_draw_char(gfx_x,gfx_y,c,gfx_fg,gfx_bg);gfx_x+=8;
    if(gfx_x>=fb.width-8){gfx_x=8;gfx_y+=16;if(gfx_y>=fb.height-16)gfx_scroll();}
}

void gfx_print(const char*s){for(int i=0;s[i];i++)gfx_putchar(s[i]);}
void gfx_println(const char*s){gfx_print(s);gfx_putchar('\n');}
void gfx_set_color(uint32_t f,uint32_t b){gfx_fg=f;gfx_bg=b;}
void gfx_clear(){fb_clear(gfx_bg);gfx_x=8;gfx_y=8;}
void gfx_print_int(uint32_t n){char b[11];int i=10;b[i--]=0;do{b[i--]='0'+(n%10);n/=10;}while(n);gfx_print(&b[i+1]);}
void gfx_print_hex(uint32_t n){char h[]="0123456789ABCDEF";gfx_print("0x");for(int i=28;i>=0;i-=4)gfx_putchar(h[(n>>i)&0xF]);}

void gfx_readline(char*buf,int max){
    static const char lo[]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
    static const char up[]={0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
    int i=0,shift=0,caps=0;
    while(i<max-1){
        while(!(inb(0x64)&1))__asm__ volatile("pause");
        uint8_t sc=inb(0x60);
        if(sc==0x2A||sc==0x36){shift=1;continue;}
        if(sc==0xAA||sc==0xB6){shift=0;continue;}
        if(sc==0x3A){caps=!caps;continue;}
        if(sc&0x80)continue;
        char c=(shift^caps)?up[sc]:lo[sc];
        if(!c)continue;
        if(c=='\n')break;
        if(c=='\b'){if(i>0){i--;gfx_putchar('\b');}continue;}
        buf[i++]=c;gfx_putchar(c);
    }
    buf[i]='\0';gfx_putchar('\n');
}

int strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return*a-*b;}
int strlen(const char*s){int i=0;while(s[i])i++;return i;}
int startswith(const char*s,const char*p){while(*p)if(*s++!=*p++)return 0;return 1;}

void cmd_help(){
    uint32_t c=0xAAAAAA,v=0x88CC88,r=0xCC88AA,w=0xDDDDDD;
    gfx_set_color(v,gfx_bg);gfx_println("");gfx_println("  FreeARS - all commands");
    gfx_set_color(c,gfx_bg);gfx_println("  =========================================");
    gfx_set_color(w,gfx_bg);
    gfx_println("  help              show this message");
    gfx_println("  clear             clear the screen");
    gfx_println("  uname             system info");
    gfx_println("  echo <text>       print text");
    gfx_println("  sleep <ms>        sleep milliseconds");
    gfx_println("  memtest           test kernel heap");
    gfx_println("  pagetest          test virtual memory");
    gfx_println("  crash             test exception handler");
    gfx_println("  ticks             show system ticks");
    gfx_println("  fastfetch         system info display");
    gfx_println("  mkfile <n> <c>    create file");
    gfx_println("  cat <file>        show file content");
    gfx_println("  ls                list files");
    gfx_println("  rm <file>         remove file");
    gfx_println("  arpm list         list packages");
    gfx_println("  arpm -ci <pkg>    install a package");
    gfx_set_color(r,gfx_bg);gfx_println("");gfx_println("  Shift/Caps Lock supported!");gfx_println("");
}

void cmd_uname(){gfx_set_color(0xFFFF,gfx_bg);gfx_println("  FreeARS 0.03 - x86_64 64-bit");}
void cmd_echo(const char*a){if(!a[0]){gfx_set_color(0xFFFF00,gfx_bg);gfx_println("  echo: nothing");return;}gfx_set_color(0xFF00,gfx_bg);gfx_print("  ");gfx_println(a);}
void cmd_sleep(const char*a){if(!a[0]){gfx_set_color(0xFFFF00,gfx_bg);gfx_println("  usage: sleep <ms>");return;}uint32_t ms=0;for(int i=0;a[i];i++){if(a[i]>='0'&&a[i]<='9')ms=ms*10+a[i]-'0';else{gfx_println("  invalid");return;}}gfx_set_color(0xFFFF,gfx_bg);gfx_print("  Sleeping ");gfx_print_int(ms);gfx_println(" ms...");sleep_ms(ms);gfx_set_color(0xFF00,gfx_bg);gfx_println("  Done!");}
void cmd_memtest(){void*a=kmalloc(128),*b=kmalloc(64);kfree(a);void*c=kmalloc(32);gfx_set_color(c&&b?0xFF00:0xFF0000,gfx_bg);gfx_println(c&&b?"  kmalloc ok!":"  kmalloc failed!");kfree(b);kfree(c);}
void cmd_pagetest(){volatile uint64_t*p=(uint64_t*)0x300000;*p=0xDEADBEEF;gfx_set_color(*p==0xDEADBEEF?0xFF00:0xFF0000,gfx_bg);gfx_println(*p==0xDEADBEEF?"  paging ok!":"  paging failed!");}
void cmd_ticks(){gfx_set_color(0xFFFF,gfx_bg);gfx_print("  ticks: ");gfx_set_color(0xFFFFFF,gfx_bg);gfx_print_int(timer_get_ticks());gfx_println("");}

void cmd_crash(){
    gfx_set_color(0xFF0000,gfx_bg);gfx_println("  Crashing in 3...");sleep_ms(500);
    gfx_println("  2...");sleep_ms(500);gfx_println("  1...");sleep_ms(500);
    gfx_println("  CRASH!");
    volatile int a=10,b=0,c=a/b;(void)c;
}

static const char*pkgs[]={"slide","hello","calc",0};
int pkg_exists(const char*n){for(int i=0;pkgs[i];i++)if(!strcmp(pkgs[i],n))return 1;return 0;}
void cmd_arpm(const char*a){
    if(!strcmp(a,"list")){gfx_set_color(0xFFFF,gfx_bg);gfx_println("  packages:");for(int i=0;pkgs[i];i++){gfx_set_color(0xFFFF00,gfx_bg);gfx_print("    - ");gfx_set_color(0xFFFFFF,gfx_bg);gfx_println(pkgs[i]);}return;}
    if(startswith(a,"-ci ")){const char*p=a+4;if(!pkg_exists(p)){gfx_set_color(0xFF0000,gfx_bg);gfx_print("  not found: ");gfx_println(p);return;}gfx_set_color(0xFFFF,gfx_bg);gfx_print("  installing ");gfx_print(p);gfx_println("...");gfx_set_color(0xFF00,gfx_bg);gfx_println("  done.");return;}
    gfx_set_color(0xFFFF00,gfx_bg);gfx_println("  usage: arpm list | arpm -ci <pkg>");
}

void cmd_fastfetch(){
    gfx_clear();
    uint32_t cy=0x88AACC,w=0xDDDDDD,g=0x88CC88,gr=0xAAAAAA,r=0xCC88AA,y=0xCCCC88,b=0x8888CC;
    
    // Logo
    gfx_set_color(cy,gfx_bg);
    gfx_println("   ______                        _____   _____ ");
    gfx_println("  |  ____|                 /\\   |  __ \\ / ____|");
    gfx_println("  | |__ _ __ ___  ___     /  \\  | |__) | (___  ");
    gfx_set_color(w,gfx_bg);
    gfx_println("  |  __| '__/ _ \\/ _ \\   / /\\ \\ |  _  / \\___ \\ ");
    gfx_println("  | |  | | |  __/  __/  / ____ \\| | \\ \\ ____) |");
    gfx_set_color(cy,gfx_bg);
    gfx_println("  |_|  |_|  \\___|\\___| /_/    \\_\\_|  \\_\\_____/ ");
    gfx_println("");
    
    // Usuário
    gfx_set_color(w,gfx_bg);gfx_print("  ");
    gfx_set_color(r,gfx_bg);gfx_println("user@FreeARS");
    gfx_set_color(gr,gfx_bg);gfx_println("  -----------");
    
    // OS
    gfx_set_color(w,gfx_bg);gfx_print("  OS:       ");
    gfx_set_color(g,gfx_bg);gfx_println("FreeARS 0.03");
    
    // Kernel
    gfx_set_color(w,gfx_bg);gfx_print("  Kernel:   ");
    gfx_set_color(g,gfx_bg);gfx_println("x86_64 64-bit");
    
    // Shell
    gfx_set_color(w,gfx_bg);gfx_print("  Shell:    ");
    gfx_set_color(g,gfx_bg);gfx_println("fsh 0.3");
    
    // Uptime
    uint32_t t=timer_get_ticks()/100;
    uint32_t h=t/3600,m=(t%3600)/60,s=t%60;
    gfx_set_color(w,gfx_bg);gfx_print("  Uptime:   ");
    gfx_set_color(g,gfx_bg);
    if(h){gfx_print_int(h);gfx_print("h ");}
    if(m){gfx_print_int(m);gfx_print("m ");}
    gfx_print_int(s);gfx_println("s");
    
    // RAM total (do GRUB Multiboot2)
    uint32_t total_ram = 256; // padrão
    uint8_t *tags = (uint8_t *)0x10000; // MBI aproximado
    // Procura tag de memória (tipo 6)
    uint32_t *tag = (uint32_t *)(tags + 8);
    while (1) {
        uint32_t type = tag[0];
        uint32_t size = tag[1];
        if (type == 0) break;
        if (type == 6) {
            total_ram = (uint32_t)(tag[2] / 1024); // KB -> MB
        }
        tag = (uint32_t *)((uint8_t *)tag + ((size + 7) / 8) * 8);
    }
    gfx_set_color(w,gfx_bg);gfx_print("  RAM:      ");
    gfx_set_color(g,gfx_bg);
    gfx_print_int(total_ram);gfx_println(" MB");
    
    gfx_set_color(w,gfx_bg);gfx_print("  RAM Disk: ");
    gfx_set_color(g,gfx_bg);
    gfx_print_int(fs_ls());gfx_println(" files");
    
    char cpu_name[49] = {0};
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax) : "a"(0x80000002));
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(0x80000002));
    *(uint32_t *)(cpu_name + 0) = eax;
    *(uint32_t *)(cpu_name + 4) = ebx;
    *(uint32_t *)(cpu_name + 8) = ecx;
    *(uint32_t *)(cpu_name + 12) = edx;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(0x80000003));
    *(uint32_t *)(cpu_name + 16) = eax;
    *(uint32_t *)(cpu_name + 20) = ebx;
    *(uint32_t *)(cpu_name + 24) = ecx;
    *(uint32_t *)(cpu_name + 28) = edx;
    __asm__ volatile("cpuid" : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx) : "a"(0x80000004));
    *(uint32_t *)(cpu_name + 32) = eax;
    *(uint32_t *)(cpu_name + 36) = ebx;
    *(uint32_t *)(cpu_name + 40) = ecx;
    *(uint32_t *)(cpu_name + 44) = edx;
    cpu_name[48] = '\0';
    int start = 0;
    while (cpu_name[start] == ' ') start++;
    gfx_set_color(w,gfx_bg);gfx_print("  CPU:      ");
    gfx_set_color(g,gfx_bg);
    gfx_println(&cpu_name[start]);

    gfx_set_color(w,gfx_bg);gfx_print("  GPU:      ");
    gfx_set_color(g,gfx_bg);gfx_println("VESA/VBE Compatible");
    gfx_set_color(w,gfx_bg);gfx_print("  Display:  ");
    gfx_set_color(g,gfx_bg);

    gfx_print_int(fb.width);gfx_print("x");gfx_print_int(fb.height);gfx_println(" VESA");
    gfx_set_color(w,gfx_bg);gfx_print("  Input:    ");
    gfx_set_color(g,gfx_bg);gfx_println("PS/2 Keyboard (polling)");
    gfx_println("");
    
    uint32_t bc[]={r,y,g,cy,b};
    for(int row=0;row<5;row++){
        gfx_print("  ");
        gfx_set_color(bc[row],gfx_bg);
        for(int col=0;col<30;col++)gfx_print("#");
        gfx_println("");
    }
    
    gfx_println("");
    gfx_set_color(gr,gfx_bg);
    gfx_println("  Type 'help' for available commands.");
    gfx_println("");
}

void cmd_mkfile(const char *args){
    char name[32],content[256];int i=0,j=0;
    while(args[i]==' ')i++;
    while(args[i]&&args[i]!=' '&&j<31)name[j++]=args[i++];
    name[j]='\0';
    if(name[0]=='\0'){gfx_set_color(0xFF0000,gfx_bg);gfx_println("  usage: mkfile <name> <content>");return;}
    while(args[i]==' ')i++;
    j=0;while(args[i]&&j<255)content[j++]=args[i++];
    content[j]='\0';
    if(fs_mkfile(name,content)==0){gfx_set_color(0x00FF00,gfx_bg);gfx_print("  Created: ");gfx_println(name);}
    else{gfx_set_color(0xFF0000,gfx_bg);gfx_println("  FS full!");}
}

void cmd_cat(const char *name){
    if(name[0]=='\0'){gfx_set_color(0xFF0000,gfx_bg);gfx_println("  usage: cat <file>");return;}
    int idx=fs_get_index(name);
    if(idx>=0){gfx_set_color(0x00FF00,gfx_bg);gfx_print("  ");gfx_println(fs_get_content(idx));}
    else{gfx_set_color(0xFF0000,gfx_bg);gfx_print("  not found: ");gfx_println(name);}
}

void cmd_ls(){
    int count=fs_ls();
    if(count==0){gfx_set_color(0x888888,gfx_bg);gfx_println("  (empty)");return;}
    gfx_set_color(0x00FFFF,gfx_bg);gfx_println("  files:");
    for(int i=0;i<32;i++){char*n=fs_get_name(i);if(n){gfx_set_color(0x00FF00,gfx_bg);gfx_print("    - ");gfx_set_color(0xFFFFFF,gfx_bg);gfx_println(n);}}
}

void cmd_rm(const char *name){
    if(name[0]=='\0'){gfx_set_color(0xFF0000,gfx_bg);gfx_println("  usage: rm <file>");return;}
    if(fs_rm(name)==0){gfx_set_color(0x00FF00,gfx_bg);gfx_print("  Removed: ");gfx_println(name);}
    else{gfx_set_color(0xFF0000,gfx_bg);gfx_print("  not found: ");gfx_println(name);}
}

void gfx_shell(){
    char in[256];
    while(1){
        gfx_set_color(0xFF00,gfx_bg);gfx_print("freeARS> ");
        gfx_set_color(0xFFFFFF,gfx_bg);gfx_readline(in,256);
        if(!strcmp(in,"help"))cmd_help();
        else if(!strcmp(in,"clear"))gfx_clear();
        else if(!strcmp(in,"uname"))cmd_uname();
        else if(startswith(in,"echo"))cmd_echo(strlen(in)>5?in+5:"");
        else if(startswith(in,"sleep"))cmd_sleep(strlen(in)>6?in+6:"");
        else if(!strcmp(in,"memtest"))cmd_memtest();
        else if(!strcmp(in,"pagetest"))cmd_pagetest();
        else if(!strcmp(in,"crash"))cmd_crash();
        else if(!strcmp(in,"ticks"))cmd_ticks();
        else if(!strcmp(in,"fastfetch"))cmd_fastfetch();
        else if(startswith(in,"arpm"))cmd_arpm(strlen(in)>5?in+5:"");
        else if(startswith(in,"mkfile"))cmd_mkfile(strlen(in)>7?in+7:"");
        else if(startswith(in,"cat"))cmd_cat(strlen(in)>4?in+4:"");
        else if(!strcmp(in,"ls"))cmd_ls();
        else if(startswith(in,"rm"))cmd_rm(strlen(in)>3?in+3:"");
        else if(in[0]){gfx_set_color(0xFF0000,gfx_bg);gfx_print("  not found: ");gfx_println(in);}
    }
}

void kernel_main(uint64_t magic, uint64_t mbi){
    paging_init();kmalloc_init();idt_init();
    pic_remap(0x20,0x28);timer_init(100);
    outb(0x21,inb(0x21)|2);__asm__ volatile("sti");
    fb_init(magic,mbi);
    fs_init();
    if(!fb.available){
        uint32_t ta[]={0xFD000000,0xE0000000,0xC0000000,0xD0000000,0x90000000,0x80000000};
        for(int i=0;i<6;i++){
            fb.address=(uint32_t*)ta[i];fb.width=800;fb.height=600;fb.pitch=3200;fb.bpp=32;
            volatile uint32_t*t=fb.address;uint32_t bk=*t;*t=0x00FF0000;
            if(*t==0x00FF0000){*t=bk;fb.available=1;break;}
        }
    }
    if(fb.available){
        gfx_bg=0x00111122;gfx_clear();gfx_x=8;gfx_y=8;
        gfx_set_color(0x88CC,gfx_bg);
        gfx_println("   ______                        _____   _____ ");
        gfx_println("  |  ____|                 /\\   |  __ \\ / ____|");
        gfx_println("  | |__ _ __ ___  ___     /  \\  | |__) | (___  ");
        gfx_set_color(0xFFFFFF,gfx_bg);
        gfx_println("  |  __| '__/ _ \\/ _ \\   / /\\ \\ |  _  / \\___ \\ ");
        gfx_println("  | |  | | |  __/  __/  / ____ \\| | \\ \\ ____) |");
        gfx_set_color(0x88CC,gfx_bg);
        gfx_println("  |_|  |_|  \\___|\\___| /_/    \\_\\_|  \\_\\_____/ ");
        gfx_println("                                               ");
        gfx_set_color(0x888888,gfx_bg);
        gfx_println("  FreeARS 0.03 -- Another Random System");
        gfx_println("  type 'help' for available commands.");
        gfx_println("");gfx_shell();
    }else{
        volatile unsigned short*v=(unsigned short*)0xB8000;
        for(int i=0;i<80*25;i++)v[i]=0x0720;
        char*b="FreeARS 0.03 - VGA Fallback";
        for(int i=0;b[i];i++)v[i]=b[i]|0x0700;
        for(;;)__asm__ volatile("hlt");
    }
}