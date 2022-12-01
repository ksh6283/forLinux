#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define BUFF_SIZE 1024
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct ListNode
{
    int data;
    struct ListNode *link;
} ListNode;
ListNode *head = NULL;
void push(ListNode **list, int item)
{
    ListNode *temp = (ListNode *)malloc(sizeof(ListNode));
    temp->data = item;
    temp->link = NULL;
    ListNode *pos = *list;
    if (*list == NULL)
    {
        *list = temp;
    }
    else if (temp->data <= (*list)->data)
    {
        temp->link = *list;
        *list = temp;
    }
    else
    {
        while (1)
        {
            if (pos->link == NULL)
            {
                pos->link = temp;
                break;
            }
            else if (temp->data < pos->link->data)
            {
                temp->link = pos->link;
                pos->link = temp;
                break;
            }
            pos = pos->link;
        }
    }
}
int lsearch(ListNode* head, int x)
{
    ListNode* temp = head; 
    while (temp != NULL) {
        if (temp->data == x)
            return 1;
        temp = temp->link;
    }
    return 0;
}

void itoa(int num, char *str){
    int i=0;
    int temp=1;
    int digit = 0;

    if(num==0){
        str[0]='0';
        str[1]='\n';
        str[2]='\0';
        return;
    }

    while(1){   
        if( (num/temp) > 0)
            digit++;
        else
            break;
        temp *= 10;
    }
    temp /=10;    
    for(i=0; i<digit; i++)    {    
        *(str+i) = num/temp + '0';    
        num -= ((num/temp) * temp);       
        temp /=10;    
    }
    *(str+i) = '\n';
    *(str+i+1) = '\0'; 
} 

typedef struct
{
    int fd;
    int tnum;
    int len;  //탐색할 row의 수
    int lLen; //마지막 스레드가 탐색할 row의 수
    int line;
} for_search;

char *str; 

void *search(void *arg)
{

    

    int temp = 0;
    int slen = strlen(str);
    int flag = slen; 
    int start = 0;
    int rsize;

    for_search *targ = (for_search *)arg;

    char buf[500001];
   
    if(targ->len==0&&targ->lLen==0)
        return 0;


    off_t pos;

    pos = 6 + (off_t)((targ->tnum) * (targ->len)) * 6;

    
    if ((targ->tnum) != 0)
    {
        
        if(slen>1&&(targ->len)!=0){
            pos -= slen +(slen-2)/5;
            rsize = (targ->lLen) * 5 + slen +(slen-2)/5+(slen - 1);
        }else if((targ->len)==0){
            pos-=slen;
            rsize=(targ->lLen)*5+slen;
        }else{
            pos-=slen-1;
            rsize=(targ->lLen) * 5 + slen -1+(slen - 1);
        }
            
       
        
    }
    else
    {
        rsize = (targ->len) * 5+slen -1;
    }

   

    
    for (int i = 0; i < rsize; i++)
    {

        if (pread(targ->fd, buf + sizeof(char) * i, 1, pos) == 0) 
        {
            break;

        }
            
        pos += 1;
        if (buf[i] == '\n')
        {
            i -= 1;
        }
    }
   
    for (int i = 0; i < sizeof(buf); i++)
    {

        if (buf[i] != str[temp])
        { 
            flag = slen;
            temp = 0;
            start = 0;
        }
        if (buf[i] == str[temp])
        {
            if (temp == 0)
                start = i;
            flag -= 1;
            temp += 1;
        }

        if (flag == 0)
        {
            start += 5 * (targ->len) * (targ->tnum);
            if ((targ->tnum) != 0)
                start -= slen-1;
            pthread_mutex_lock(&mutex);
            if(!lsearch(head,start)) 
                push(&head,start);
            pthread_mutex_unlock(&mutex); 
            temp = 0;
            flag = slen;
        }
    }
    return 0;
}





int main(int argc, char **argv)
{

    if (argc != 5)
    {
        exit(EXIT_FAILURE);
    }

    char *input, *output;
    str = argv[1];
    int tnum = atoi(argv[2]); 
    input = argv[3];
    output = argv[4];
    
    
    int fd, line, t_status;
    pthread_t  thread_id[tnum];

    char cline[6];



    fd = open(input, O_RDONLY);
    read(fd, cline, 6);
    line = atoi(cline); 

    for_search arg[tnum]; 
    for (int i = 0; i < tnum; i++)
    {
        arg[i].fd = fd;
        arg[i].tnum = i;
        arg[i].len = (int)(line / tnum);
        if (i == tnum - 1) 
            arg[i].lLen = line - (tnum - 1) * (int)(line / tnum);
        else
            arg[i].lLen = (int)(line / tnum);
        arg[i].line=line;
    }
    for (int i = 0; i < tnum; i++)
    {
        t_status = pthread_create(&thread_id[i], NULL, search, &arg[i]);
        if (t_status != 0)
        {
            perror("pthread_create\n");
            exit(1);
        }
    }
    for (int i = 0; i < tnum; i++)
    {
        t_status = pthread_join(thread_id[i], 0);
        if (t_status != 0)
        {
            perror("join");
            exit(1);
        }
    }
    pthread_mutex_destroy(&mutex); 

    int fd2 = open(output, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
    if ( fd2 < 0 ){
        perror("open");
        exit(1);
    }
    ListNode *temp = head;
    for (; temp != NULL; temp = temp->link)
    {
        char str[100001];
        itoa((temp->data),str);
        if ( write(fd2, str, strlen(str)) < 0 ){
            perror("write");
            close(fd);
            exit(1);
        }
        
    }
    close(fd);
    close(fd2);

    while (head != NULL)
    {
       temp = head;
       head = head->link;
       free(temp);
    }
    
}