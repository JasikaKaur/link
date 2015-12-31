/* c program for the implementation of rsa algorithm encrypt the text data and decrypt the same */

#include<stdio.h>


int phi,m,n,e,d,c,flag;

int check()
{
int i;
for(i=3;e%i==0 && phi%i==0;i+2)
{
flag = 1;
return;
}
flag = 0;
}

void encrypt()
{
int i;
c = 1;
for(i=0;i< e;i++)
c=c*m%n;
c = c%n;
printf(“\n\tencrypted keyword : %d”,c);
}

void decrypt()
{
int i;
m = 1;
for(i=0;i< d;i++)
m=m*c%n;
m = m%n;
printf(“\n\tdecrypted keyword : %d”,m);
}

void main()
{
int p,q,s;
clrscr();
printf(“enter two relatively prime numbers\t: “);
scanf(“%d%d”,&p,&q);
n = p*q;
phi=(p-1)*(q-1);
printf(“\n\tf(n) phi value\t= %d”,phi);
do
{
printf(“\n\nenter e which is prime number and less than phi \t: “,n);
scanf(“%d”,&e);
check();
}while(flag==1);
d = 1;
do
{
s = (d*e)%phi;
d++;
}while(s!=1);
d = d-1;
printf(“\n\tpublic key\t: {%d,%d}”,e,n);
printf(“\n\tprivate key\t: {%d,%d}”,d,n);
printf(“\n\nenter the plain text\t: “);
scanf(“%d”,&m);
encrypt();
printf(“\n\nenter the cipher text\t: “);
scanf(“%d”,&c);
decrypt();
getch();
}
