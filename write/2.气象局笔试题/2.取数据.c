#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef struct D{
    char data[20];
}D_t;

typedef struct N{
    D_t value;
    struct N *next;
}N_t;

//创建链表
N_t * create_link(N_t * head, D_t d){
    N_t *p = (N_t *)malloc(sizeof(N_t));
    p->value = d;
    p->next = NULL;
    if(head ==NULL){
        return p;
    }else if(head){
        N_t *tail = head;
        while(tail->next)
            tail = tail->next;
        tail->next = p;
    }
    return head;
}

N_t * read_data(N_t *head, char *filename){
    //读取一行数据
    char buf[1024] = {};
    //保存一行中一个数据
    D_t d;
    memset(d.data, 0 , sizeof(d.data));
    FILE *f = fopen(filename, "r");
    fscanf(f, "%s", buf);
    int i = 0, j = 0, k = 0;
    for(; i < strlen(buf); i++){
        if(buf[i] != ','){
            d.data[k++] = buf[i];
        }else if(buf[i] == ','){
            d.data[k++] = '\0';
            head = create_link(head, d);
            memset(d.data, 0, sizeof(d.data));
            j++;
            k = 0;
        }
        //获取最后一个字符串
        if(i == strlen(buf) - 1){
            head = create_link(head, d);
            fclose(f);
            return head;
        }
    }
}

//打印链表
void show_link(N_t *head, int n){
    N_t *s = head;
    while(s){
        for(int i = 0; i < n; i++){
            printf("%s ", s->value.data);
            s = s->next;
            if( s == NULL){
                printf("\n");
                return ;
            }
        }
        printf("\n");
    }   
}

//释放链表
void free_link(N_t *head){
    N_t *s  = NULL;
    while(head){
        s = head;
        head = head->next;
        free(s);
    }
}

void show_data(N_t *head, N_t *head2, char *filename, char *filename2, int n){
    char buf[1024] = {};
    D_t d ={};
    FILE *f = fopen(filename, "r");
    fscanf(f, "%s", buf);
    memset(buf, 0, sizeof(buf));

    //生成.xml文件
    FILE *fd = fopen(filename2, "w");
    fprintf(fd, "<data>\n");
    while(1){
        int i = 0, j = 0, k = 0;
        int res = fscanf(f, "%s", buf);//从第二行开始
        if(res == 1){
            for(;i < strlen(buf); i++){
                if(buf[i] != ','){
                    d.data[k++] = buf[i];
                }else if(buf[i] == ','){
                    d.data[k++] = '\0';
                    head2 = create_link(head2, d);
                    memset(d.data, 0, sizeof(d.data));
                    j++;
                    k = 0;
                }
                //获取最后一个字符串
                if(i == strlen(buf) - 1){
                    head2 = create_link(head2, d);
                }
            }
        }else{
            break;
        }
    }
    //输出到.xml文件
    N_t *s2 = head2;
    while(s2){
        N_t *s = head;
        for(int i = 0; i < n; i++){
            fprintf(fd, "<%s>%s</%s>",
                    s->value.data,s2->value.data,s->value.data);
            s = s->next;
            if(s2)//如果节点不是空
                s2 = s2->next;
        }
        fprintf(fd, "\n");
    }
    show_link(head2, n);
    fprintf(fd, "</data>\n");
    fclose(fd);
}

int main(){
    N_t *head = NULL;
    //获取字段
    head = read_data(head, "test.txt");
    show_link(head, 7);

    //将数据生成.xml文件
    N_t *head2 = NULL;
    //传参：字段链表头，内容链表头，待解析文件，生成.xml文件，每行内容列数
    show_data(head, head2, "test.txt", "test.xml", 7);

    //释放链表
    free_link(head);
    free_link(head2);
    return 0;
}

