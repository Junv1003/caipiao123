#include<stdio.h>
#include<unistd.h>

static char numArr[] = {18,5,26,9,12,23,15,6,10,2};

int main(void){
    int i;
    //int len=sizeof(numArr);
    //printf("%d\n",len);
    while(1){
        printf("中奖彩票号码：\n");
        for(i=0;i<10;i++){
            printf("%d ",numArr[i]);
        }
        printf("\n");
        sleep(5);
    }
    return 0;
}