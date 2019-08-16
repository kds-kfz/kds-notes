#include<stdio.h>
#include<string.h>
# if 0
    深圳市神农信息技术有限公司
    编程题目：
    机上有test.txt文件，内容为逗号分隔，中文的一行为字段名，第2-n行为内容行
    要求：解析test.txt文件，生成xml格式文件，xml格式如下
    <data>
    <字段1>内容1</字段1><字段2>内容2</字段2>...
    <字段1>内容1</字段1><字段2>内容2</字段2>...
    <字段1>内容1</字段1><字段2>内容2</字段2>...
    <字段1>内容1</字段1><字段2>内容2</字段2>...
    </data>
    举例说明：
    <name>孙小小</name><ename>along</ename><dwcode>2</dwcode><job>工程师</job><mobileno>15758495254</mobileno><comm></comm><crttime>2017091102010</crttime>
    <name>陈晓明</name><ename>beYond</ename><dwcode>54</dwcode><job>电工</job><mobileno>13758468211</mobileno><comm></comm><crttime>2017091252000</crttime>
#endif

typedef struct{
    char nam[20];
    char ena[20];
    char dwc[20];
    char job[20];
    char mob[20];
    char com[20];
    char crt[20];
}H_t;

void read_data(FILE *f, H_t *t){
    char *p[7] = {t->nam, t->ena, t->dwc, t->job, t->mob, t->com, t->crt};
    char buf[1024] = {};
    fscanf(f, "%s", buf);
    printf("%s\n",buf);
    int i = 0, j = 0, k = 0;
    for(; i < strlen(buf); i++){
        if(buf[i] != ','){
            p[j][k++] = buf[i];
        }else if(buf[i] == ',' || buf[i] == '\n'){
            p[j][k++] = '\0';
            j++;
            k = 0;
        }else{
            break;
        }
    }
}

void show_data(FILE *f, H_t *t, H_t *t2){
    char *p[7] = {t->nam, t->ena, t->dwc, t->job, t->mob, t->com, t->crt};
    char buf[1024] = {};
    FILE *fd = fopen("test.xml", "w");
    fprintf(fd, "<data>\n");
    while(1){
        for(int w = 0; w < 7; w++)
            memset(p[w], 0, sizeof(p[w]));
        int res = fscanf(f, "%s", buf);
        if(res == 1){
            int i = 0, j = 0, k = 0;
            for(; i < strlen(buf); i++){
                if(buf[i] != ','){
                    p[j][k++] = buf[i];
                }else if(buf[i] == ','){
                    p[j][k++] = '\0';
                    j++;
                    k = 0;
                }else{
                    break;
                }
            }
            //生成xml文件
            fprintf(fd, "<%s>%s</%s><%s>%s</%s><%s>%s</%s><%s>%s</%s><%s>%s</%s><%s>%s</%s><%s>%s</%s>\n",
                        t2->nam,p[0],t2->nam,
                        t2->ena,p[1],t2->ena,
                        t2->dwc,p[2],t2->dwc,
                        t2->job,p[3],t2->job,
                        t2->mob,p[4],t2->mob,
                        t2->com,p[5],t2->com,
                        t2->crt,p[6],t2->crt);
            //完成一次数据清空缓存
        }else{
            break;
        }
    }
    fprintf(fd, "</data>\n");
    fclose(fd);
}
int main(){
    H_t t;
    FILE *f = fopen("test.txt","r");
    //获取字段
    read_data(f, &t);

    //将数据生成test.xml文件
    H_t t2;
    show_data(f, &t2, &t);
    fclose(f);
    return 0;
}
