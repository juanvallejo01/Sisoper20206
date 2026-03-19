/* memory.c (ANSI C89) - Estático/Dinámico (Best/Worst) + VM (Paginación + Segmentación bonus)
   Compilar: gcc -std=c89 -Wall -Wextra -pedantic memory.c -o memory
*/
#include <stdio.h>
#include <stdbool.h>

/* ---- Config ---- */
#define MEM_SIZE 1024
#define MAX_BLK  64

#define NPART 8
static const int PART_SZ[NPART] = {64,64,128,128,128,160,160,192};

#define PAGE_SZ 64
#define NFRAMES (MEM_SIZE / PAGE_SZ)
#define MAX_PROC 16
#define MAX_PAGES 32

#define SEG_MAX_PROC 16
#define SEG_MAX_SEG  8
#define SEG_LOG_SZ   1024

/* ---- Dynamic blocks ---- */
typedef struct { int pid, size; bool used; } DBlock;
static DBlock dmem[MAX_BLK];
static int dcnt;

/* ---- Static partitions ---- */
typedef struct { int pid, size; bool used; } Part;
static Part spart[NPART];

/* ---- Paging ---- */
typedef struct { int pid, np, bytes; int pt[MAX_PAGES]; } PProc;
static bool frames[NFRAMES];
static PProc pproc[MAX_PROC];
static int pcnt;

/* ---- Segmentation ---- */
typedef struct { int base, lim; bool used; } Seg;
typedef struct { int pid; Seg s[SEG_MAX_SEG]; } SProc;
static SProc sproc[SEG_MAX_PROC];
static int scnt;

/* ---- IO helpers ---- */
static void flush_in(void)
{
    int c;
    do { c = getchar(); } while (c != '\n' && c != EOF);
}

static int rd(const char *p)
{
    int x;
    printf("%s", p);
    if (scanf("%d", &x) != 1) {
        printf("Entrada inválida.\n");
        flush_in();
        return 0;
    }
    return x;
}

/* ===================== Dynamic partitioning ===================== */
static void dinit(void)
{
    dmem[0].pid = -1;
    dmem[0].size = MEM_SIZE;
    dmem[0].used = false;
    dcnt = 1;
}

static void dprint(void)
{
    int i;
    puts("\n-- Dinámico --");
    for (i = 0; i < dcnt; i++)
        printf("Bloque %2d: %s (%d bytes)\n", i, dmem[i].used ? "OCUP" : "LIBRE", dmem[i].size);
}

static void dmerge(void)
{
    int i, k;
    for (i = 0; i < dcnt - 1; i++) {
        if (!dmem[i].used && !dmem[i + 1].used) {
            dmem[i].size += dmem[i + 1].size;
            for (k = i + 1; k < dcnt - 1; k++) dmem[k] = dmem[k + 1];
            dcnt--;
            i--;
        }
    }
}

static void dsplit(int idx, int sz)
{
    int rem, j;
    rem = dmem[idx].size - sz;
    dmem[idx].size = sz;

    if (rem > 0) {
        if (dcnt >= MAX_BLK) { dmem[idx].size += rem; return; }
        for (j = dcnt; j > idx + 1; j--) dmem[j] = dmem[j - 1];
        dmem[idx + 1].pid = -1; dmem[idx + 1].size = rem; dmem[idx + 1].used = false;
        dcnt++;
    }
}

static void dalloc(int pid, int sz, int best)
{
    int i, pick, score;
    pick = -1;
    score = best ? (MEM_SIZE + 1) : -1;

    if (sz <= 0) { puts("Tamaño inválido."); return; }

    for (i = 0; i < dcnt; i++) if (!dmem[i].used && dmem[i].size >= sz) {
        int diff = dmem[i].size - sz;
        if ((best && diff < score) || (!best && dmem[i].size > score)) {
            score = best ? diff : dmem[i].size;
            pick = i;
        }
    }

    if (pick < 0) { printf("%s FIT (dinámico): sin espacio.\n", best ? "BEST" : "WORST"); return; }

    dsplit(pick, sz);
    dmem[pick].pid = pid;
    dmem[pick].used = true;
    printf("%s FIT (dinámico): pid=%d asignado %d bytes.\n", best ? "BEST" : "WORST", pid, sz);
}

static void dfreep(int pid)
{
    int i;
    for (i = 0; i < dcnt; i++) if (dmem[i].used && dmem[i].pid == pid) {
        dmem[i].used = false;
        dmem[i].pid = -1;
        dmerge();
        printf("Dinámico: pid=%d liberado.\n", pid);
        return;
    }
    puts("Dinámico: pid no encontrado.");
}

/* ===================== Static partitioning ===================== */
static void sinit(void)
{
    int i;
    for (i = 0; i < NPART; i++) { spart[i].pid = -1; spart[i].size = PART_SZ[i]; spart[i].used = false; }
}

static void sprint(void)
{
    int i;
    puts("\n-- Estático (particiones fijas) --");
    for (i = 0; i < NPART; i++)
        printf("Part %2d: %s (%d bytes)\n", i, spart[i].used ? "OCUP" : "LIBRE", spart[i].size);
}

static void salloc(int pid, int sz, int best)
{
    int i, pick, score;
    pick = -1;
    score = best ? (MEM_SIZE + 1) : -1;

    if (sz <= 0) { puts("Tamaño inválido."); return; }

    for (i = 0; i < NPART; i++) if (!spart[i].used && spart[i].size >= sz) {
        int diff = spart[i].size - sz;
        if ((best && diff < score) || (!best && spart[i].size > score)) {
            score = best ? diff : spart[i].size;
            pick = i;
        }
    }

    if (pick < 0) { printf("%s FIT (estático): no hay partición.\n", best ? "BEST" : "WORST"); return; }

    spart[pick].used = true;
    spart[pick].pid = pid;
    printf("%s FIT (estático): pid=%d -> part=%d (part=%d, frag_int=%d)\n",
           best ? "BEST" : "WORST", pid, pick, spart[pick].size, spart[pick].size - sz);
}

static void sfreep(int pid)
{
    int i;
    for (i = 0; i < NPART; i++) if (spart[i].used && spart[i].pid == pid) {
        spart[i].used = false;
        spart[i].pid = -1;
        printf("Estático: pid=%d liberado.\n", pid);
        return;
    }
    puts("Estático: pid no encontrado.");
}

/* ===================== Paging ===================== */
static void pinit(void)
{
    int i;
    for (i = 0; i < NFRAMES; i++) frames[i] = false;
    pcnt = 0;
    puts("Paginación inicializada.");
}

static void pframes(void)
{
    int i;
    puts("\n-- Marcos --");
    for (i = 0; i < NFRAMES; i++) printf("Frame %2d: %s\n", i, frames[i] ? "OCUP" : "libre");
}

static void ptables(void)
{
    int i, p;
    puts("\n-- Tablas de páginas --");
    if (pcnt == 0) puts("(sin procesos)");
    for (i = 0; i < pcnt; i++) {
        printf("pid=%d np=%d bytes=%d frag_int=%d\n",
               pproc[i].pid, pproc[i].np, pproc[i].bytes, pproc[i].np * PAGE_SZ - pproc[i].bytes);
        for (p = 0; p < pproc[i].np; p++) printf("  pág %2d -> frame %2d\n", p, pproc[i].pt[p]);
    }
}

static void palloc(int pid, int bytes)
{
    int np, freec, i, as;
    PProc *e;

    if (bytes <= 0) { puts("Tamaño inválido."); return; }

    np = (bytes + PAGE_SZ - 1) / PAGE_SZ;
    if (pcnt >= MAX_PROC) { puts("Paginación: límite de procesos."); return; }
    if (np > MAX_PAGES) { puts("Paginación: excede MAX_PAGES."); return; }

    freec = 0;
    for (i = 0; i < NFRAMES; i++) if (!frames[i]) freec++;
    if (freec < np) { puts("Paginación: no hay marcos suficientes."); return; }

    e = &pproc[pcnt];
    e->pid = pid; e->np = np; e->bytes = bytes;

    as = 0;
    for (i = 0; i < NFRAMES && as < np; i++) if (!frames[i]) { frames[i] = true; e->pt[as++] = i; }

    pcnt++;
    printf("Paginación: pid=%d asignado %d bytes -> %d pág (frag_int=%d)\n",
           pid, bytes, np, np * PAGE_SZ - bytes);
}

static void pfreep(int pid)
{
    int i, p, k;
    for (i = 0; i < pcnt; i++) if (pproc[i].pid == pid) {
        for (p = 0; p < pproc[i].np; p++) frames[pproc[i].pt[p]] = false;
        for (k = i; k < pcnt - 1; k++) pproc[k] = pproc[k + 1];
        pcnt--;
        printf("Paginación: pid=%d liberado.\n", pid);
        return;
    }
    puts("Paginación: pid no encontrado.");
}

static void ptrans(int pid, int la)
{
    int i, pg, off;
    pg = la / PAGE_SZ; off = la % PAGE_SZ;

    for (i = 0; i < pcnt; i++) if (pproc[i].pid == pid) {
        if (la < 0 || pg >= pproc[i].np) { puts("Traducción: fuera de rango."); return; }
        printf("pid=%d L=%d -> pág=%d off=%d -> frame=%d -> F=%d\n",
               pid, la, pg, off, pproc[i].pt[pg], pproc[i].pt[pg] * PAGE_SZ + off);
        return;
    }
    puts("Traducción: pid no encontrado.");
}

/* ===================== Segmentation (bonus) ===================== */
static void sgm_init(void) { scnt = 0; puts("Segmentación inicializada."); }

static SProc *sgm_find(int pid)
{
    int i;
    for (i = 0; i < scnt; i++) if (sproc[i].pid == pid) return &sproc[i];
    return NULL;
}

static SProc *sgm_get(int pid)
{
    int i;
    SProc *p = sgm_find(pid);
    if (p) return p;
    if (scnt >= SEG_MAX_PROC) return NULL;

    sproc[scnt].pid = pid;
    for (i = 0; i < SEG_MAX_SEG; i++) { sproc[scnt].s[i].used = false; sproc[scnt].s[i].base = 0; sproc[scnt].s[i].lim = 0; }
    scnt++;
    return &sproc[scnt - 1];
}

static void sgm_make(int pid, int sid, int sz)
{
    SProc *p;
    int i, end;

    if (sz <= 0 || sid < 0 || sid >= SEG_MAX_SEG) { puts("Segmentación: datos inválidos."); return; }
    p = sgm_get(pid); if (!p) { puts("Segmentación: límite de procesos."); return; }

    end = 0;
    for (i = 0; i < SEG_MAX_SEG; i++) if (p->s[i].used) {
        int e = p->s[i].base + p->s[i].lim;
        if (e > end) end = e;
    }
    if (end + sz > SEG_LOG_SZ) { puts("Segmentación: sin espacio lógico."); return; }

    p->s[sid].base = end; p->s[sid].lim = sz; p->s[sid].used = true;
    printf("Segmentación: pid=%d seg=%d base=%d lim=%d\n", pid, sid, p->s[sid].base, p->s[sid].lim);
}

static void sgm_trans(int pid, int sid, int off)
{
    SProc *p = sgm_find(pid);
    if (!p) { puts("Segmentación: pid no encontrado."); return; }
    if (sid < 0 || sid >= SEG_MAX_SEG || !p->s[sid].used) { puts("Segmentación: segmento inexistente."); return; }
    if (off < 0 || off >= p->s[sid].lim) { puts("Segmentación: violación de límite."); return; }
    printf("pid=%d (seg=%d,off=%d) -> dir=%d\n", pid, sid, off, p->s[sid].base + off);
}

static void sgm_print(void)
{
    int i, s;
    puts("\n-- Tablas de segmentos --");
    if (scnt == 0) puts("(sin procesos)");
    for (i = 0; i < scnt; i++) {
        printf("pid=%d\n", sproc[i].pid);
        for (s = 0; s < SEG_MAX_SEG; s++) if (sproc[i].s[s].used)
            printf("  seg %d: base=%d lim=%d\n", s, sproc[i].s[s].base, sproc[i].s[s].lim);
    }
}

/* ===================== Menus ===================== */
static void menu_static(void)
{
    int op, pid, sz;
    sinit();
    for (;;) {
        puts("\n[ESTÁTICO] 1 BestFit alloc | 2 WorstFit alloc | 3 free(pid) | 4 print | 0 back");
        op = rd("Opción: "); if (op == 0) return;
        if (op == 1 || op == 2) { pid = rd("pid: "); sz = rd("bytes: "); salloc(pid, sz, op == 1); }
        else if (op == 3) { pid = rd("pid: "); sfreep(pid); }
        else if (op == 4) sprint();
        else puts("Inválida.");
    }
}

static void menu_dynamic(void)
{
    int op, pid, sz;
    dinit();
    for (;;) {
        puts("\n[DINÁMICO] 1 BestFit alloc | 2 WorstFit alloc | 3 free(pid) | 4 print | 0 back");
        op = rd("Opción: "); if (op == 0) return;
        if (op == 1 || op == 2) { pid = rd("pid: "); sz = rd("bytes: "); dalloc(pid, sz, op == 1); }
        else if (op == 3) { pid = rd("pid: "); dfreep(pid); }
        else if (op == 4) dprint();
        else puts("Inválida.");
    }
}

static void menu_paging(void)
{
    int op, pid, bytes, la;
    pinit();
    for (;;) {
        puts("\n[PAGING] 1 alloc(pid,bytes) | 2 free(pid) | 3 frames | 4 tables | 5 translate | 0 back");
        op = rd("Opción: "); if (op == 0) return;
        if (op == 1) { pid = rd("pid: "); bytes = rd("bytes: "); palloc(pid, bytes); }
        else if (op == 2) { pid = rd("pid: "); pfreep(pid); }
        else if (op == 3) pframes();
        else if (op == 4) ptables();
        else if (op == 5) { pid = rd("pid: "); la = rd("dir lógica: "); ptrans(pid, la); }
        else puts("Inválida.");
    }
}

static void menu_seg(void)
{
    int op, pid, sid, sz, off;
    sgm_init();
    for (;;) {
        puts("\n[SEG] 1 create(pid,seg,size) | 2 translate(pid,seg,off) | 3 print | 0 back");
        op = rd("Opción: "); if (op == 0) return;
        if (op == 1) { pid = rd("pid: "); sid = rd("seg(0..7): "); sz = rd("size: "); sgm_make(pid, sid, sz); }
        else if (op == 2) { pid = rd("pid: "); sid = rd("seg: "); off = rd("off: "); sgm_trans(pid, sid, off); }
        else if (op == 3) sgm_print();
        else puts("Inválida.");
    }
}

int main(void)
{
    int op;
    printf("SIMULADOR MEMORIA | MEM=%d | PAGE=%d | FRAMES=%d\n", MEM_SIZE, PAGE_SZ, NFRAMES);
    for (;;) {
        puts("\nMENÚ: 1 Estático | 2 Dinámico | 3 Paginación | 4 Segmentación(bonus) | 0 Salir");
        op = rd("Opción: "); if (op == 0) break;
        if (op == 1) menu_static();
        else if (op == 2) menu_dynamic();
        else if (op == 3) menu_paging();
        else if (op == 4) menu_seg();
        else puts("Inválida.");
    }
    return 0;
}