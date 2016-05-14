 /* main7-3.c ����bo7-3.c�������� */
 #include"c1.h"
 typedef char InfoType;
 #define MAX_Info 80 /* ��Ϣ�ַ�����󳤶�+1 */
 #define MAX_VERTEX_NAME 3  /* �����ַ�����󳤶�+1 */
 typedef char  VertexType[MAX_VERTEX_NAME];
 #include"c7-3.h"
 #include"bo7-3.c"
 Status visit(VertexType v)
 {
   printf("%s ",v);
   return OK;
 }

 void main()
 {
   int j,k,n;
   OLGraph g;
   VertexType v1,v2;
   CreateDG(&g);
   Display(g);
   printf("�޸Ķ����ֵ��������ԭֵ ��ֵ: ");
   scanf("%s%s",v1,v2);
   PutVex(&g,v1,v2);
   printf("�����¶��㣬�����붥���ֵ: ");
   scanf("%s",v1);
   InsertVex(&g,v1);
   printf("�������¶����йصĻ��������뻡��: ");
   scanf("%d",&n);
   for(k=0;k<n;k++)
   {
     printf("��������һ�����ֵ ��һ����ķ���(0:��ͷ 1:��β): ");
     scanf("%s%d",v2,&j);
     if(j)
       InsertArc(&g,v2,v1);
     else
       InsertArc(&g,v1,v2);
   }
   Display(g);
   printf("ɾ��һ�������������ɾ�����Ļ�β ��ͷ��");
   scanf("%s%s",v1,v2);
   DeleteArc(&g,v1,v2);
   Display(g);
   printf("ɾ�����㼰��صĻ��������붥���ֵ: ");
   scanf("%s",v1);
   DeleteVex(&g,v1);
   Display(g);
   printf("������������Ľ��:\n");
   DFSTraverse(g,visit);
   printf("������������Ľ��:\n");
   BFSTraverse(g,visit);
   DestroyGraph(&g);
 }