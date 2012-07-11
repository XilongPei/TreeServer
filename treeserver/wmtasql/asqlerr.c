#include "asqlana.h"
#include "ASQLERR.H"
#include "cfg_fast.h"

char *AsqlErrorMes( void )
{
    switch( qsError ) {
	case 1001:	return "内存空间使用错";
	case 1002:	return "读语法文件时内存错";
	case 1003:	return "查询对象识别时内存错";
	case 1004:	return "动作(ACTION)处理时内存错";
	case 1005:	return "预定义(PREDICATE)时内存申请错";
	case 1006:	return "动作处理时内存错";
	case 1007:	return "表达式处理时内存错";
	case 1008:	return "表达式执行时内存错";
	case 1009:	return "动作表达式(ACTION)书写错";

	case 1010:	return "数组定义出错,下标定义中有字母或大小不合适";
	case 1011:	return "数组定义出错,维数太多";
	case 1012:	return "TEXT定义时内存申请出错";

	case 2001:	return "语法文件打不开";
	case 2002:	return "语法文件读错";
	case 2003:	return "查询对象(FROM)打开错";
	case 2004:      return "查询对象(FROM)中(SCOPE:key,start,end)描述错";
	case 2005:      return "SCOPE:描述中关键字错";
	case 2006:	return "DATABASE:描述错";
	case 2007:	return "no right of from tables";
	case 2008:	return "FROM中别名命名错，应以字母开头";
	case 3001:	return "注释太长";
	case 3002:	return "字符串未结束";
	case 3003:	return "表达式未结束";
	case 3004:	return "无统计声明或其位置不对";
	case 3005:	return "统计描述时表格项指示数字太大";
	case 3006:	return "符号识别错";
	case 3007:	return "动作识别错";
	case 3008:	return "字符串书写错，双引号不可以紧邻相遇";
	case 3014:	return "变量定义(PREDICATES)出错";
	case 3016:	return "目标(TO)描述错";
	case 3017:	return "不知名关键字或BEGIN END不匹配";
	case 3018:	return "该出现关键字处无";
	case 3019:	return "查询目标(TO)描述错";
	case 3020:	return "预定义(PREDICATE)时不知明数据类型";
	case 3021:	return "预定义(PREDICATE)时变量名太长";
	case 3022:	return "条件(CONDITION)后无BEGIN";
	case 3023:	return "条件(CONDITION)中出现不知名关键字";
	case 3024:	return "条件(CONDITION)中BEGIN与END不对齐";
	case 3025:	return "目标域名源库中不存在";
	case 3026:	return "动作表达式出现不知名符号";
	case 3027:	return "条件表达式中出现不知明变量或函数";
	case 3028:	return "查询目标(TO)中域名错或无','号分隔";
	case 3029:	return "ORDERBY描述错，无法进行排序";
	case 3030:	return "不可识别的关键字";

	case 3101:	return "表结构修改(MODISTRU)描述错";
	case 3102:	return "表结构修改(MODISTRU)语法格式为：表名,结构定义";
	case 3103:	return "表结构修改(MODISTRU)中表打开错";
	case 3104:	return "表结构修改(MODISTRU)中表无法独占打开";
	case 3105:	return "表结构修改(MODISTRU)域描述错";
	case 3106:	return "表结构修改(MODISTRU)无法进行(可能原因:域名重复,盘空间不足)";

	case 4001:	return "表达式分析时内存错";
	case 4002:	return "表达式书写错";
	case 4003:	return "描述性动作分析错";
	case 4004:	return "条件(CONDITION)一般性动作分析错";
	case 4005:	return "统计中没有设定统计要求";
	case 4006:	return "条件(CONDITION)只出现动作表达式而无条件表达式";
	case 5001:	return "警告:目标文件不存在";
	case 5002:	return "目标文件描述或创建错";
	case 5003:	return "分组(Group By)语法错,分组关键字太长";
	case 5004:	return "条件动作最后一遍执行错";
	case 5005:	return "描述动作执行错";
	case 5006:	return "条件动作第一遍执行错";
	case 5007:	return "内部错，函数引用错";
	case 5008:      return "分组索引无法建立";
	case 5009:	return "dSetAwake()出错";
	case 5010:	return "分组(Group By)索引引用错";

	case 6001:	return "内部错";
	case 6002:	return "FROM:关联关系描述错";
	case 6004:	return "创建连接索引时错，可能是连接关键字描述错";
	//case 6005:	return "从源中读取记录时错";
	case 6006:	return "每个ASQL表达式必须以关键字开头";

	case 7001:	return "errorStamp";
	case 7002:	return "取记录时出错";
	case 7003:	return "ENDIF与IF不匹配";
	case 7004:	return "ENDWHILE与WHILE不匹配";
	case 7005:	return "LOOP与WHILE不匹配";
	case 7006:	return "CALL函数但函数不存在";
	case 7007:      return "execGramMan()出错";
	case 7008:	return "ENDIF或ELSE或ENDWHILE前必须有Asql_LET";
	case 7009:	return "ELSE与ENDIF不匹配";
	case 7010:	return "ENDTEXT与TEXT定义关键字不匹配";

	case 9001:      return "目标表已经被独占打开";
    }
    return "内部错";

} // end of function AsqlErrorMes()



