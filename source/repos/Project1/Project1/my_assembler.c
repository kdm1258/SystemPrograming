/*
 * 파일명 : my_assembler.c
 * 설  명 : 이 프로그램은 SIC/XE 머신을 위한 간단한 Assembler 프로그램의 메인루틴으로,
 * 입력된 파일의 코드 중, 명령어에 해당하는 OPCODE를 찾아 출력한다.
 *
 */

 /*
  *
  * 프로그램의 헤더를 정의한다.
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

  // 파일명의 "00000000"은 자신의 학번으로 변경할 것.
#include "my_assembler_20211397.h"

/* -----------------------------------------------------------------------------------
 * 설명 : 사용자로 부터 어셈블리 파일을 받아서 명령어의 OPCODE를 찾아 출력한다.
 * 매계 : 실행 파일, 어셈블리 파일
 * 반환 : 성공 = 0, 실패 = < 0
 * 주의 : 현재 어셈블리 프로그램의 리스트 파일을 생성하는 루틴은 만들지 않았다.
 *		   또한 중간파일을 생성하지 않는다.
 * -----------------------------------------------------------------------------------
 */


int main(int args, char* arg[])
{
	if (init_my_assembler() < 0)
	{
		printf("init_my_assembler: 프로그램 초기화에 실패 했습니다.\n");
		return -1;
	}

	if (assem_pass1() < 0)
	{
		printf("assem_pass1: 패스1 과정에서 실패하였습니다.  \n");
		return -1;
	}

	make_opcode_output(NULL);	// 기계어 코드 출력

	// make_symtab_output("output_symtab.txt");         //  추후 과제에 사용 예정
	// make_literaltab_output("output_littab.txt");     //  추후 과제에 사용 예정

	if (assem_pass2() < 0)
	{
		printf(" assem_pass2: 패스2 과정에서 실패하였습니다.  \n");
		return -1;
	}

	// make_objectcode_output("output_objectcode.txt"); //  추후 과제에 사용 예정
}

/* -----------------------------------------------------------------------------------
 * 설명 : 프로그램 초기화를 위한 자료구조 생성 및 파일을 읽는 함수이다.
 * 매계 : 없음
 * 반환 : 정상종료 = 0 , 에러 발생 = -1
 * 주의 : 각각의 명령어 테이블을 내부에 선언하지 않고 관리를 용이하게 하기
 *		   위해서 파일 단위로 관리하여 프로그램 초기화를 통해 정보를 읽어 올 수 있도록
 *		   구현하였다.
 * -----------------------------------------------------------------------------------
 */
int init_my_assembler(void)
{
	int result;

	if ((result = init_inst_file("inst_table.txt")) < 0)
		return -1;
	if ((result = init_input_file("input.txt")) < 0)
		return -1;
	return result;
}

/* ----------------------------------------------------------------------------------
 * 설명 : 머신을 위한 기계 코드목록 파일(inst_table.txt)을 읽어
 *       기계어 목록 테이블(inst_table)을 생성하는 함수이다.
 *
 *
 * 매계 : 기계어 목록 파일
 * 반환 : 정상종료 = 0 , 에러 < 0
 * 주의 : 기계어 목록파일 형식은 자유롭게 구현한다. 예시는 다음과 같다.
 *
 *	===============================================================================
 *		   | 이름 | 형식 | 기계어 코드 | 오퍼랜드의 갯수 | \n |
 *	===============================================================================
 *
 * ----------------------------------------------------------------------------------
 */
int init_inst_file(char* inst_file)
{
	FILE* file;
	int errno;
	char buffer[255];

	errno = fopen_s(&file, inst_file, "r");
	if (errno != 0)
	{
		printf("파일 열기 실패\n");
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	while (fgets(buffer, sizeof(buffer), file) != NULL)
	{
		inst* new_inst = (inst*)malloc(sizeof(inst));
		if (new_inst == NULL)
		{
			printf("메모리 할당 실패\n");
			return -1;
		}

		unsigned int op_tmp;
		char format[10];

		if (sscanf_s(buffer, "%s\t%s\t%x\t%d",
			new_inst->str, (unsigned int)sizeof(new_inst->str),
			format, (unsigned int)sizeof(format),
			&op_tmp,
			&new_inst->ops) != 4) {
			printf("inst_table.txt 파일을 파싱하는데 실패했습니다.\n");
			return -1;
		}
		//opcode 저장
		new_inst->op = (unsigned char)op_tmp;

		/*
		 * format 저장 :
		 * format == "3/4" 이면 일단 ops == 3을 저장하고, Input_data에서 + 를 만나면 ops == 4로 업데이트 한다.
		 */
		if (format[1] == '/') {
			new_inst->format = 3;
		}
		else {
			new_inst->format = format[0] - '0';
		}

		// inst_table에 저장
		inst_table[inst_index++] = new_inst;
	}
	fclose(file);
	return errno;
}

/* ----------------------------------------------------------------------------------
 * 설명 : 어셈블리 할 소스코드 파일(input.txt)을 읽어 소스코드 테이블(input_data)를 생성하는 함수이다.
 * 매계 : 어셈블리할 소스파일명
 * 반환 : 정상종료 = 0 , 에러 < 0
 * 주의 : 라인단위로 저장한다.
 *
 * ----------------------------------------------------------------------------------
 */
int init_input_file(char* input_file)
{
	FILE* file;
	int errno;
	char buffer[MAX_LINE_LENGTH];
	errno = fopen_s(&file, input_file, "r");

	if (errno != 0)
	{
		printf("파일 열기 실패\n");
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	// 파일을 라인 단위로 읽어 input_data에 저장
	// MAXLINES을 넘을 경우 중단
	while (fgets(buffer, sizeof(buffer), file) != NULL && line_num < MAX_LINES)
	{
		char* temp = _strdup(buffer);
		if (temp == NULL) {
			printf("메모리 할당 실패 at line %d\n", line_num);
			break;
		}
		input_data[line_num++] = temp;
	}
	// MAXLINES을 넘을 경우 에러
	if (line_num >= MAX_LINES) {
		printf("최대 라인수를 초과하였습니다.\n");
		return -1;
	}
	fclose(file);
	return errno;
}

/* ----------------------------------------------------------------------------------
 * 설명 : 소스 코드를 읽어와 토큰단위로 분석하고 토큰 테이블을 작성하는 함수이다.
 *        패스 1로 부터 호출된다.
 * 매계 : 파싱을 원하는 문자열
 * 반환 : 정상종료 = 0 , 에러 < 0
 * 주의 : my_assembler 프로그램에서는 라인단위로 토큰 및 오브젝트 관리를 하고 있다.
 * ----------------------------------------------------------------------------------
 */
int token_parsing(char* str)
{
	// 임시토큰 초기화
	token* tok = (token*)malloc(sizeof(token));
	if (tok == NULL) {
		printf("메모리 할당 실패\n");
		return -1;
	}
	tok->label = NULL;
	tok->oper = NULL;
	for (int i = 0; i < MAX_OPERAND; i++)	tok->operand[i] = NULL;
	tok->comment[0] = '\0';

	// 주석처리
	if (str[0] == '.') {
		strcpy_s(tok->comment, sizeof(tok->comment), str);
		token_table[token_line] = tok;
		token_line++;
		return 0;
	}

	char* tmp_label = NULL;
	char* tmp_oper = NULL;
	char* tmp_str = _strdup(str);
	char* tmp_operands = NULL;
	char* tmp_comment = NULL;
	char* context = NULL;	// strtok_s()의 context 변수
	int num_operands = MAX_OPERAND;	// 피연산자 개수


	// label이 없는경우 : '\t'로 시작
	if (str[0] == '\t') {
		tmp_str = tmp_str + 1;
		tmp_oper = strtok_s(tmp_str, " \t\n", &context);
	}
	//label이 있는 경우
	else {
		tmp_label = strtok_s(tmp_str, " \t\n", &context);
		tmp_oper = strtok_s(NULL, " \t\n", &context);
	}

	if (tmp_oper != NULL) {
		tok->oper = _strdup(tmp_oper);

		// operator가 NULL이 아닌경우 최대 피연산자 개수를 가져온다
		for (int i = 0; i < inst_index; i++) {
			const char* opcheck = (tmp_oper[0] == '+') ? tmp_oper + 1 : tmp_oper;
			if (strcmp(inst_table[i]->str, opcheck) == 0) {
				num_operands = inst_table[i]->ops;
				break;
			}
		}
	}
	// operand 개수가 0 ex) RSUB인 경우에는 나머지 문자열을 모두 comment처리한다.
	if (num_operands > 0)	tmp_operands = strtok_s(NULL, " \t\n", &context);
	tmp_comment = strtok_s(NULL, "", &context);	//남은 문자열을 모두 주석으로 처리
	// label과 존재시 할당
	if (tmp_label != NULL) {
		tok->label = _strdup(tmp_label);
	}

	/* operand parsing :
	 * operand는 ','로 구분되어 있다.
	 * tmp_operands로 따로 구분한 이유는 comment의 ','와 구분하기 위해서이다.
	 */
	if (tmp_operands != NULL) {
		char* operand_context = NULL;	// strtok_s()의 context 변수
		char* tmp_operand = strtok_s(tmp_operands, ",", &operand_context);
		int i = 0;
		while (tmp_operand != NULL && i < MAX_OPERAND) {
			tok->operand[i++] = _strdup(tmp_operand);
			tmp_operand = strtok_s(NULL, ",", &operand_context);
		}
	}

	// comment는 ','와 상관없이 통째로 저장
	if (tmp_comment != NULL) {
		//comment 공백제거
		while (*tmp_comment == ' ' || *tmp_comment == '\t') tmp_comment++;
		strcpy_s(tok->comment, sizeof(tok->comment), tmp_comment);
	}

	token_table[token_line++] = tok;
	return 0;
}



/* ----------------------------------------------------------------------------------
* 설명 : 어셈블리 코드를 위한 패스1과정을 수행하는 함수이다.
*		   패스1에서는..
*		   1. 프로그램 소스를 스캔하여 해당하는 토큰단위로 분리하여 프로그램 라인별 토큰
*		   테이블을 생성한다.
*          2. 토큰 테이블은 token_parsing()을 호출하여 설정한다.
*          3. assem_pass2 과정에서 사용하기 위한 심볼테이블 및 리터럴 테이블을 생성한다.
*
*
*
* 매계 : 없음
* 반환 : 정상 종료 = 0 , 에러 = < 0
* 주의 : 현재 초기 버전에서는 에러에 대한 검사를 하지 않고 넘어간 상태이다.
*	     따라서 에러에 대한 검사 루틴을 추가해야 한다.
*
*        OPCODE 출력 프로그램에서는 심볼테이블, 리터럴테이블을 생성하지 않아도 된다.
*        그러나, 추후 프로젝트 1을 수행하기 위해서는 심볼테이블, 리터럴테이블이 필요하다.
*
* -----------------------------------------------------------------------------------
*/
static int assem_pass1(void)
{
	/* input_data의 문자열을 한줄씩 입력 받아서
	 * token_parsing()을 호출하여 _token에 저장
	 */
	for (int i = 0; i < line_num; i++) {
		if (token_parsing(input_data[i]) < 0) {
			printf("token_parsing: 토큰 파싱에 실패하였습니다.\n");
			return -1;
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------------------
 * 설명 : 입력 문자열이 기계어 코드인지를 검사하는 함수이다.
 * 매계 : 토큰 단위로 구분된 문자열
 * 반환 : 정상종료 = 기계어 테이블 인덱스, 에러 < 0
 * 주의 : 기계어 목록 테이블에서 특정 기계어를 검색하여, 해당 기계어가 위치한 인덱스를 반환한다.
 *        '+JSUB'과 같은 문자열에 대한 처리는 자유롭게 처리한다.
 *
 * ----------------------------------------------------------------------------------
 */
int search_opcode(char* str)
{
	// operator가 NULL인 경우
	if (str == NULL)
	{
		return -1;
	}
	//4형식 처리 : +로 시작
	if (str[0] == '+')
	{
		str++;	// +를 제거


	}
	for (int i = 0; i < inst_index; i++)
	{
		if (strcmp(inst_table[i]->str, str) == 0)
		{
			return i;
		}
	}
	return -1;	// 찾지 못한 경우
}

/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 소스코드 명령어 앞에 OPCODE가 기록된 코드를 파일에 출력한다.
*        파일이 NULL값이 들어온다면 프로그램의 결과를 stdout으로 보내어
*        화면에 출력해준다.
*
*        OPCODE 출력 프로그램의 최종 output 파일을 생성하는 함수이다.
*        (추후 프로젝트 1에서는 불필요)
*
* -----------------------------------------------------------------------------------
*/
void make_opcode_output(char* file_name)
{

	FILE* file;
	int errno;
	if (file_name == NULL)
	{
		file = stdout;
	}
	else {
		errno = fopen_s(&file, file_name, "w");
		if (errno != 0)
		{
			printf("파일 열기 실패\n");
			return;
		}
	}

	fseek(file, 0, SEEK_SET);

	for (int i = 0; i < token_line; i++)
	{
		token* t = token_table[i];

		// 주석 라인
		if (t->oper == NULL)
		{
			fprintf(file, "%s\n", t->comment);
			continue;
		}

		// operand 문자열 만들기 (보안 버전)
		char operand_str[100] = "";
		for (int j = 0; j < MAX_OPERAND; j++) {
			if (t->operand[j] != NULL) {
				if (operand_str[0] != '\0') {
					strcat_s(operand_str, sizeof(operand_str), ",");
				}
				strcat_s(operand_str, sizeof(operand_str), t->operand[j]);
			}
		}

		// opcode 문자열 만들기 (보안 버전)
		char opcode_str[10] = "";
		int idx = search_opcode(t->oper);
		if (idx != -1) {
			sprintf_s(opcode_str, sizeof(opcode_str), "%02X", inst_table[idx]->op);
		}

		// 정렬된 출력
		fprintf(file, "%-8s%-8s%-16s%-8s\n",
			t->label ? t->label : "",
			t->oper ? t->oper : "",
			operand_str,
			opcode_str);
	}
	if (file != stdout)
	{
		fclose(file);
	}
	return;
}


/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 SYMBOL별 주소값이 저장된 TABLE이다.
* 매계 : 생성할 오브젝트 파일명 혹은 경로
* 반환 : 없음
* 주의 : 파일이 NULL값이 들어온다면 프로그램의 결과를 stdout으로 보내어
*        화면에 출력해준다.
*
* -----------------------------------------------------------------------------------
*/
void make_symtab_output(char* file_name)
{
	/* add your code here */
}


/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 LITERAL별 주소값이 저장된 TABLE이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 파일이 NULL값이 들어온다면 프로그램의 결과를 stdout으로 보내어
*        화면에 출력해준다.
*
* -----------------------------------------------------------------------------------
*/
void make_literaltab_output(char* filename)
{
	/* add your code here */
}


/* -----------------------------------------------------------------------------------
 * 설명 : 어셈블리 코드를 기계어 코드로 바꾸기 위한 패스2 과정을 수행하는 함수이다.
 *		   패스 2에서는 프로그램을 기계어로 바꾸는 작업은 라인 단위로 수행된다.
 *		   다음과 같은 작업이 수행되어 진다.
 *		   1. 실제로 해당 어셈블리 명령어를 기계어로 바꾸는 작업을 수행한다.
 * 매계 : 없음
 * 반환 : 정상종료 = 0, 에러발생 = < 0
 * 주의 :
 * -----------------------------------------------------------------------------------
 */

static int assem_pass2(void)
{

	/* add your code here */

}

/* ----------------------------------------------------------------------------------
* 설명 : 입력된 문자열의 이름을 가진 파일에 프로그램의 결과를 저장하는 함수이다.
*        여기서 출력되는 내용은 object code이다.
* 매계 : 생성할 오브젝트 파일명
* 반환 : 없음
* 주의 : 파일이 NULL값이 들어온다면 프로그램의 결과를 stdout으로 보내어
*        화면에 출력해준다.
*        명세서의 주어진 출력 결과와 완전히 동일해야 한다.
*        예외적으로 각 라인 뒤쪽의 공백 문자 혹은 개행 문자의 차이는 허용한다.
*
* -----------------------------------------------------------------------------------
*/
void make_objectcode_output(char* file_name)
{
	/* add your code here */
}

