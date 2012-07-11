#include "asqlana.h"
#include "ASQLERR.H"
#include "cfg_fast.h"

char *AsqlErrorMes( void )
{
    switch( qsError ) {
	case 1001:	return "�ڴ�ռ�ʹ�ô�";
	case 1002:	return "���﷨�ļ�ʱ�ڴ��";
	case 1003:	return "��ѯ����ʶ��ʱ�ڴ��";
	case 1004:	return "����(ACTION)����ʱ�ڴ��";
	case 1005:	return "Ԥ����(PREDICATE)ʱ�ڴ������";
	case 1006:	return "��������ʱ�ڴ��";
	case 1007:	return "���ʽ����ʱ�ڴ��";
	case 1008:	return "���ʽִ��ʱ�ڴ��";
	case 1009:	return "�������ʽ(ACTION)��д��";

	case 1010:	return "���鶨�����,�±궨��������ĸ���С������";
	case 1011:	return "���鶨�����,ά��̫��";
	case 1012:	return "TEXT����ʱ�ڴ��������";

	case 2001:	return "�﷨�ļ��򲻿�";
	case 2002:	return "�﷨�ļ�����";
	case 2003:	return "��ѯ����(FROM)�򿪴�";
	case 2004:      return "��ѯ����(FROM)��(SCOPE:key,start,end)������";
	case 2005:      return "SCOPE:�����йؼ��ִ�";
	case 2006:	return "DATABASE:������";
	case 2007:	return "no right of from tables";
	case 2008:	return "FROM�б���������Ӧ����ĸ��ͷ";
	case 3001:	return "ע��̫��";
	case 3002:	return "�ַ���δ����";
	case 3003:	return "���ʽδ����";
	case 3004:	return "��ͳ����������λ�ò���";
	case 3005:	return "ͳ������ʱ�����ָʾ����̫��";
	case 3006:	return "����ʶ���";
	case 3007:	return "����ʶ���";
	case 3008:	return "�ַ�����д��˫���Ų����Խ�������";
	case 3014:	return "��������(PREDICATES)����";
	case 3016:	return "Ŀ��(TO)������";
	case 3017:	return "��֪���ؼ��ֻ�BEGIN END��ƥ��";
	case 3018:	return "�ó��ֹؼ��ִ���";
	case 3019:	return "��ѯĿ��(TO)������";
	case 3020:	return "Ԥ����(PREDICATE)ʱ��֪����������";
	case 3021:	return "Ԥ����(PREDICATE)ʱ������̫��";
	case 3022:	return "����(CONDITION)����BEGIN";
	case 3023:	return "����(CONDITION)�г��ֲ�֪���ؼ���";
	case 3024:	return "����(CONDITION)��BEGIN��END������";
	case 3025:	return "Ŀ������Դ���в�����";
	case 3026:	return "�������ʽ���ֲ�֪������";
	case 3027:	return "�������ʽ�г��ֲ�֪����������";
	case 3028:	return "��ѯĿ��(TO)�����������','�ŷָ�";
	case 3029:	return "ORDERBY�������޷���������";
	case 3030:	return "����ʶ��Ĺؼ���";

	case 3101:	return "��ṹ�޸�(MODISTRU)������";
	case 3102:	return "��ṹ�޸�(MODISTRU)�﷨��ʽΪ������,�ṹ����";
	case 3103:	return "��ṹ�޸�(MODISTRU)�б�򿪴�";
	case 3104:	return "��ṹ�޸�(MODISTRU)�б��޷���ռ��";
	case 3105:	return "��ṹ�޸�(MODISTRU)��������";
	case 3106:	return "��ṹ�޸�(MODISTRU)�޷�����(����ԭ��:�����ظ�,�̿ռ䲻��)";

	case 4001:	return "���ʽ����ʱ�ڴ��";
	case 4002:	return "���ʽ��д��";
	case 4003:	return "�����Զ���������";
	case 4004:	return "����(CONDITION)һ���Զ���������";
	case 4005:	return "ͳ����û���趨ͳ��Ҫ��";
	case 4006:	return "����(CONDITION)ֻ���ֶ������ʽ�����������ʽ";
	case 5001:	return "����:Ŀ���ļ�������";
	case 5002:	return "Ŀ���ļ������򴴽���";
	case 5003:	return "����(Group By)�﷨��,����ؼ���̫��";
	case 5004:	return "�����������һ��ִ�д�";
	case 5005:	return "��������ִ�д�";
	case 5006:	return "����������һ��ִ�д�";
	case 5007:	return "�ڲ����������ô�";
	case 5008:      return "���������޷�����";
	case 5009:	return "dSetAwake()����";
	case 5010:	return "����(Group By)�������ô�";

	case 6001:	return "�ڲ���";
	case 6002:	return "FROM:������ϵ������";
	case 6004:	return "������������ʱ�����������ӹؼ���������";
	//case 6005:	return "��Դ�ж�ȡ��¼ʱ��";
	case 6006:	return "ÿ��ASQL���ʽ�����Թؼ��ֿ�ͷ";

	case 7001:	return "errorStamp";
	case 7002:	return "ȡ��¼ʱ����";
	case 7003:	return "ENDIF��IF��ƥ��";
	case 7004:	return "ENDWHILE��WHILE��ƥ��";
	case 7005:	return "LOOP��WHILE��ƥ��";
	case 7006:	return "CALL����������������";
	case 7007:      return "execGramMan()����";
	case 7008:	return "ENDIF��ELSE��ENDWHILEǰ������Asql_LET";
	case 7009:	return "ELSE��ENDIF��ƥ��";
	case 7010:	return "ENDTEXT��TEXT����ؼ��ֲ�ƥ��";

	case 9001:      return "Ŀ����Ѿ�����ռ��";
    }
    return "�ڲ���";

} // end of function AsqlErrorMes()



