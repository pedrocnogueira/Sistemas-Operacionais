// compare_labs.c — Gera APENAS "compare_labs.txt" com médias + speedup
// Estrutura esperada (rode a partir de Lab05):
//   Lab03/lab3.c
//   Lab05/lab5.c
//   Lab05/compare_labs.c
//
// Compilar: gcc -O2 compare_labs.c -o compare_labs
// Rodar:    ./compare_labs
//
// Saída: "compare_labs.txt" (na pasta Lab05)

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Caminhos/flags */
static const char *PATH_LAB3_SRC = "../Lab03/lab3.c";
static const char *PATH_LAB5_SRC = "./lab5.c";
static const char *OUT_LAB3_BIN  = "./lab3_bin";
static const char *OUT_LAB5_BIN  = "./lab5_bin";
static const char *CFLAGS        = "-O2";

/* Varredura padrão (ajuste se quiser) */
static const long N_LIST[] = {10000, 1000000};
static const int  W_LIST[] = {1, 2, 4, 8, 16};

/* Ensaios e filtro (ajuste se quiser) */
static const int    WARMUP            = 1;      // execuções de aquecimento (descarta)
static const int    TRIALS            = 7;      // execuções medidas (brutas)
static const double OUTLIER_WALL_MAX  = 0.050;  // s; 0.0 desativa filtro

/* ===== util de tempo ===== */
static double now_sec(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Executa comando, descarta stdout/stderr, mede wall */
static int run_cmd_wall(const char *cmd, double *wall_s){
    double t0 = now_sec();
    int rc = system(cmd);
    double t1 = now_sec();
    if (wall_s) *wall_s = t1 - t0;
    return rc;
}

/* Executa comando, CAPTURA stdout e mede wall (pra extrair EX1/EX2 do lab5) */
static char* run_cmd_capture(const char *cmd, double *wall_s){
    double t0 = now_sec();
    FILE *fp = popen(cmd, "r");
    if(!fp) return NULL;

    size_t cap=8192, used=0;
    char *buf=(char*)malloc(cap);
    if(!buf){ pclose(fp); return NULL; }

    while(1){
        if(cap-used<1024){
            cap*=2;
            char *nb=(char*)realloc(buf,cap);
            if(!nb){ free(buf); pclose(fp); return NULL; }
            buf=nb;
        }
        size_t n=fread(buf+used,1,1024,fp);
        used+=n;
        if(n==0) break;
    }
    int rc=pclose(fp);
    double t1 = now_sec();
    if (wall_s) *wall_s = t1 - t0;

    if(rc!=0){ free(buf); return NULL; }
    buf[used]='\0';
    return buf;
}

/* Pega "tempo = X.XXXXXXs" de EX1/EX2 do stdout do lab5 */
static void parse_lab5_ex_times(const char *out, double *ex1, double *ex2){
    *ex1=-1.0; *ex2=-1.0;
    const char *p1=strstr(out,"EX1 (threads)");
    if(p1){ const char *t=strstr(p1,"tempo = "); if(t) sscanf(t,"tempo = %lf",ex1); }
    const char *p2=strstr(out,"EX2 (threads)");
    if(p2){ const char *t=strstr(p2,"tempo = "); if(t) sscanf(t,"tempo = %lf",ex2); }
}

int main(void){
    FILE *ftxt = fopen("compare_labs.txt","w");
    if(!ftxt){
        fprintf(stderr,"Erro: não consegui criar compare_labs.txt\n");
        return 1;
    }

    fprintf(ftxt,
        "COMPARACAO LAB3 (processos) vs LAB5 (threads)\n"
        "==============================================\n\n"
        "Parametros de execucao:\n"
        "  WARMUP=%d, TRIALS=%d, OUTLIER_WALL_MAX=%gs (0 desativa)\n"
        "  CFLAGS=%s\n\n",
        WARMUP, TRIALS, OUTLIER_WALL_MAX, CFLAGS
    );

    for(size_t in=0; in<sizeof(N_LIST)/sizeof(N_LIST[0]); ++in){
        long N = N_LIST[in];

        // compila lab5 (threads) com -DN
        {
            char cmd[1024];
            snprintf(cmd,sizeof(cmd),"gcc %s -pthread -DN=%ld '%s' -o '%s'",
                    CFLAGS, N, PATH_LAB5_SRC, OUT_LAB5_BIN);
            if(system(cmd)!=0){
                fprintf(stderr,"Falha ao compilar lab5 (N=%ld)\n",N);
                fprintf(ftxt,"N=%ld: ERRO ao compilar lab5\n\n",N);
                continue;
            }
        }

        fprintf(ftxt, "N = %ld\n", N);
        fprintf(ftxt, "------------------------------\n");
        fprintf(ftxt, "%-8s %-12s %-18s %-20s %-10s %-12s %-12s\n",
                "workers","trials_usados","avg_wall_threads(s)","avg_wall_processos(s)",
                "speedup","avg_EX1_thr","avg_EX2_thr");

        for(size_t iw=0; iw<sizeof(W_LIST)/sizeof(W_LIST[0]); ++iw){
            int W = W_LIST[iw];

            // compila lab3 (processos) com -DN e -DWORKERS
            {
                char cmd[1024];
                snprintf(cmd,sizeof(cmd),"gcc %s -DN=%ld -DWORKERS=%d '%s' -o '%s'",
                        CFLAGS, N, W, PATH_LAB3_SRC, OUT_LAB3_BIN);
                if(system(cmd)!=0){
                    fprintf(stderr,"Falha ao compilar lab3 (N=%ld, W=%d)\n",N,W);
                    fprintf(ftxt,"%-8d %-12s %-18s %-20s %-10s %-12s %-12s\n",
                            W, "0", "-", "-", "-", "-", "-");
                    continue;
                }
            }

            // warm-up
            for(int k=0;k<WARMUP;k++){
                char cmd1[256]; snprintf(cmd1,sizeof(cmd1),"%s >/dev/null 2>&1", OUT_LAB3_BIN);
                run_cmd_wall(cmd1,NULL);
                run_cmd_capture(OUT_LAB5_BIN,NULL);
            }

            // trials
            double thr_sum=0.0, pro_sum=0.0, ex1_sum=0.0, ex2_sum=0.0;
            int thr_cnt=0, pro_cnt=0, drop_thr=0, drop_pro=0;

            for(int t=0;t<TRIALS;t++){
                // processos
                {
                    double wall=0.0;
                    char cmd[256]; snprintf(cmd,sizeof(cmd),"%s >/dev/null 2>&1", OUT_LAB3_BIN);
                    int rc=run_cmd_wall(cmd,&wall);
                    if(rc==0){
                        if(OUTLIER_WALL_MAX>0 && wall>OUTLIER_WALL_MAX) drop_pro++;
                        else { pro_sum += wall; pro_cnt++; }
                    } else drop_pro++;
                }
                // threads
                {
                    double wall=0.0, e1=-1.0, e2=-1.0;
                    char *out=run_cmd_capture(OUT_LAB5_BIN,&wall);
                    if(out){
                        parse_lab5_ex_times(out,&e1,&e2);
                        free(out);
                        if(OUTLIER_WALL_MAX>0 && wall>OUTLIER_WALL_MAX) drop_thr++;
                        else {
                            thr_sum += wall; thr_cnt++;
                            if(e1>=0) ex1_sum += e1;
                            if(e2>=0) ex2_sum += e2;
                        }
                    } else drop_thr++;
                }
            }

            double avg_thr = (thr_cnt>0)? thr_sum/thr_cnt : 0.0;
            double avg_pro = (pro_cnt>0)? pro_sum/pro_cnt : 0.0;
            double avg_ex1 = (thr_cnt>0)? ex1_sum/thr_cnt : 0.0;
            double avg_ex2 = (thr_cnt>0)? ex2_sum/thr_cnt : 0.0;
            double speedup = (thr_cnt>0 && pro_cnt>0 && avg_thr>0.0)? (avg_pro/avg_thr) : 0.0;

            char trials_used[32];
            snprintf(trials_used,sizeof(trials_used),"%d", (thr_cnt<pro_cnt?thr_cnt:pro_cnt));

            fprintf(ftxt, "%-8d %-12s %-18.6f %-20.6f %-10.2f %-12.6f %-12.6f\n",
                    W, trials_used, avg_thr, avg_pro, speedup, avg_ex1, avg_ex2);
        }
        fprintf(ftxt,"\n");
    }

    fclose(ftxt);
    printf("OK! Gerado: compare_labs.txt\n");
    return 0;
}
