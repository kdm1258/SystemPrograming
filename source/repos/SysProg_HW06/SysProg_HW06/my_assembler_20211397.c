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
#include <stdbool.h>
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

	//make_opcode_output(NULL);	// 기계어 코드 출력

	make_symtab_output(NULL);         //"output_symtab.txt"
	make_literaltab_output(NULL);     //  "output_littab.txt"

	if (assem_pass2() < 0)
	{
		printf(" assem_pass2: 패스2 과정에서 실패하였습니다.  \n");
		return -1;
	}

	make_objectcode_output(NULL);

	return 0;
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
		if(format[1] == '/'){
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
		strcpy_s(tok->comment,sizeof(tok->comment), str);
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
		strcpy_s(tok->comment,sizeof(tok->comment), tmp_comment);
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
	csect_start_token_line[0] = 0;	// CSECT 시작 토큰 라인 저장
	bool start_encounter = false;	// START 여부
	int start_symtab = 0;	// sym table 서칭 시작 인덱스

	/* input_data의 문자열을 한줄씩 입력 받아서
	 * token_parsing()을 호출하여 _token에 저장
	 */
	for (int i = 0; i < line_num; i++) {
		if (token_parsing(input_data[i]) < 0) {
			printf("token_parsing: 토큰 파싱에 실패하였습니다.\n");
			return -1;
		}

		// 파싱된 토큰 
		token* t = token_table[i];
		t->addr = -1; // 주소 초기화
		if (!t || !t->oper) continue; // NULL 체크

		//comment line
		if (t->oper == NULL && t->comment[0] == '.') {
			// 주석 라인
			continue;
		}

		//START
		if (strcmp(t->oper, "START") == 0) {
			// START가 2회 이상 나오면 에러
			if (start_encounter) {
				printf("START 명령어가 중복되었습니다.\n");
				return -1;
			}

			// 프로그램 시작 주소를 설정
			if (t->operand[0] != NULL) {
				locctr = (int)strtol(t->operand[0], NULL, 16);
			}
			else {
				locctr = 0; // 시작주소가 없으면 locctr를 0으로 초기화
			}
			start_addr = locctr;	// 프로그램 시작 주소 저장
			start_encounter = true;	// START가 나왔음을 표시

			// START label저장
			if (t->label != NULL) {
				if (insert_symtab(start_symtab, t->label, locctr) < 0) return -1;
			}
			t->addr = locctr;	// locctr를 addr에 저장
			continue;	// START는 locctr를 증가시키지 않음
		}

		// CSECT 
		if (t->oper && strcmp(t->oper, "CSECT") == 0) {
			assign_littab_addresses();
			csect_start_symbol_num[csect_num++] = label_num; // CSECT 시작 심볼 번호 저장
			csect_start_token_line[csect_num] = i; // CSECT 시작 토큰 라인 저장
			start_symtab = label_num;	// 다음 label부터 시작
			insert_symtab(start_symtab, "", NULL); // 빈 label 섹터 구분용
			locctr = 0;
			if (t->label) insert_symtab(start_symtab, t->label, locctr); // CSECT 종료 위치 저장
			t->addr = locctr;	// locctr를 addr에 저장
			continue;	// LOCCTR 업데이트 없음
		}

		// label 저장
		if (t->label != NULL && strcmp(t->oper, "EQU") != 0 ) {
			if (insert_symtab(start_symtab, t->label, locctr) < 0) return -1;
		}

		// EQU
		if (strcmp(t->oper, "EQU") == 0) {
			int value = 0;

			// 피연산자가 '*'인 경우
			if (strcmp(t->operand[0], "*") == 0) {
				value = locctr;
			}
			// 피연산자가 숫자인 경우
			else if (isdigit(t->operand[0][0])) {
				value = atoi(t->operand[0]);
			}
			// '-'연산 처리
			else if(strchr(t->operand[0], '-') != NULL) {
				char operand_copy[100];
				strcpy_s(operand_copy, sizeof(operand_copy), t->operand[0]);

				char* left = strtok(operand_copy, "-");
				char* right = strtok(NULL, "-");

				int left_val = get_symval(start_symtab , left);
				int right_val = get_symval(start_symtab ,right);

				value = (left_val == -1 || right_val == -1) ? 0 : left_val - right_val;
			}
			// 피연산자가 심볼인 경우
			else {
				int symvalue = get_symval(start_symtab, t->operand[0]);
				value = (symvalue == -1) ? 0 : symvalue;	//심볼을 찾지 못한경우 0으로 채움
			}

			// 심볼 테이블에 저장
			if (t->label != NULL) {
				if (insert_symtab(start_symtab, t->label, value) < 0) return -1;
			}
			t->addr = locctr;	// locctr를 addr에 저장
			continue; // LOCCTR 업데이트 없음
		}

		//LTORG
		if (strcmp(t->oper, "LTORG") == 0) {
			assign_littab_addresses();
			continue;
		}

		// 리터럴 처리
		if (t->operand[0] != NULL && t->operand[0][0] == '=') {
			// 리터럴 테이블에 추가
			insert_literaltab(t->operand[0]);
		}

		t->addr = locctr;	// locctr를 addr에 저장
		
		// 지시어 처리
		//BYTE
		if (strcmp(t->oper, "BYTE") == 0) {
			int added_byte = 0;
			if (t->operand[0][0] == 'C') {
				added_byte = strlen(t->operand[0]) - 3; // C'abc' -> 3
			}
			else if (t->operand[0][0] == 'X') {
				added_byte = (strlen(t->operand[0]) - 3) / 2; // X'12' -> 2
			}
			else {
				return -1; // 잘못된 BYTE 형식
			}
			locctr += added_byte;
		}
		//WORD
		else if (strcmp(t->oper, "WORD") == 0) {
			locctr += 3;
		}
		//RESB
		else if (strcmp(t->oper, "RESB") == 0) {
			if (t->operand[0] != NULL) {
				locctr += atoi(t->operand[0]);
			}
		}
		//RESW
		else if (strcmp(t->oper, "RESW") == 0) {
			if (t->operand[0] != NULL) {
				locctr += 3 * atoi(t->operand[0]);
			}
		}
		else{
		// opcode 검색
			int idx = search_opcode(t->oper);
			if (idx >= 0) {
				if (t->oper[0] == '+') {
					// 4형식인 경우
					locctr += inst_table[idx]->format + 1;
				}
				else {
					locctr += inst_table[idx]->format;
				}
			}
		}

	}
	// locctr 업데이트
	assign_littab_addresses();
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

		// operand 문자열 만들기
		char operand_str[100] = "";
		for (int j = 0; j < MAX_OPERAND; j++) {
			if (t->operand[j] != NULL) {
				if (operand_str[0] != '\0') {
					strcat_s(operand_str, sizeof(operand_str), ",");
				}
				strcat_s(operand_str, sizeof(operand_str), t->operand[j]);
			}
		}

		// opcode 문자열 만들기 
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
	FILE* file;
	if (file_name == NULL)
		file = stdout;
	else
		// 파일 열기
		fopen_s(&file, file_name, "w");

	for (int i = 0; i < label_num; i++) {
		if(strcmp(sym_table[i].symbol, "") == 0){
			fprintf(file, "\n");
		}
		else if (sym_table[i].symbol != NULL) {
			fprintf(file, "%s\t\t%X\n", sym_table[i].symbol, sym_table[i].addr);
		}
	}

	fprintf(file, "\n");
	if (file_name != NULL)
		fclose(file);	// 파일 닫기
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
	FILE* file;
	if (filename == NULL)
		file = stdout;
	else
		// 파일 열기
		fopen_s(&file, filename, "w");

	for (int i = 0; i < literal_num ; i++) {
		if (strcmp(literal_table[i].symbol, "") == 0) {
			fprintf(file, "\n");
		}
		else if (literal_table[i].symbol != NULL) {
			char tmp[100];
			strncpy_s(tmp, sizeof(tmp),literal_table[i].symbol+3, strlen(literal_table[i].symbol) - 4);
			fprintf(file, "%s\t\t%X\n", tmp, literal_table[i].addr);
		}
	}

	fprintf(file, "\n");
	if (filename != NULL)
		fclose(file);	// 파일 닫기
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
	csect_num = 0;
	int start_symtab = 0;		// sym table 서칭 시작 인덱스
	bool mainroutine = false;	// 메인루틴 여부
	int last_head_idx = -1;		// 마지막 헤더 레코드 인덱스
	int text_length = 0;		// 텍스트 레코드 길이
	int text_start = -1;		// 텍스트 레코드 시작 주소
	char text_buffer[70] = "";	// 텍스트 레코드 버퍼


	// EXTREF, EXTDEF 목록 저장용
	char extdef_list[10][MAX_LINE_LENGTH]; int extdef_count = 0;
	char extref_list[10][MAX_LINE_LENGTH]; int extref_count = 0;

	for (int i = 0; i < token_line; i++) {
		token* t = token_table[i];
		object_code oc;
		char obj[10] = "";		// 객체 코드 버퍼

		// 주석 라인
		if (!t->oper || t->oper[0] == '.') continue;

		// START 처리
		if (t->oper != NULL && strcmp(t->oper, "START") == 0) {
			last_head_idx = obj_count;	// 헤더 레코드 인덱스 저장
			mainroutine = true;

			// start 심볼은 H로 시작
			oc.record_type = 'H';
			oc.locctr = t->addr;	// locctr 저장

			// object_code를 공백으로 초기화하고, 프로그램 이름 복사
			memset(oc.object_code, ' ', 6);  // 6칸 전부 공백
			if (t->label != NULL) {
				size_t len = strlen(t->label);
				if (len > 6) len = 6;
				memcpy(oc.object_code, t->label, len);  // 앞에서부터 복사
			}
			oc.object_code[6] = '\0';
			obj_codes[obj_count++] = oc;	// 객체 코드 저장
			continue;
		}

		// EXTDEF 처리 → D 레코드
		if (strcmp(t->oper, "EXTDEF") == 0) {
			for (int j = 0; j < MAX_OPERAND && t->operand[j]; j++) {
				strcpy_s(extdef_list[extdef_count++], MAX_LINE_LENGTH, t->operand[j]);
			}
			// EXTDEF가 끝나면 D 레코드 생성
			if (extdef_count > 0) {
				object_code drec = { 'D', -1, -1, "" };
				for (int k = 0; k < extdef_count; k++) {
					int addr = get_symval(start_symtab, extdef_list[k]);
					char name[7], entry[20];
					snprintf(name, sizeof(name), "%-6s", extdef_list[k]);       // 이름: 6자 공백 패딩
					snprintf(entry, sizeof(entry), "%s%06X", name, addr);       // 전체: 이름+주소
					strcat_s(drec.object_code, sizeof(drec.object_code), entry);
				}
				obj_codes[obj_count++] = drec;
			}

			continue;
		}

		// EXTREF 처리 → R 레코드
		if (strcmp(t->oper, "EXTREF") == 0) {
			for (int j = 0; j < MAX_OPERAND && t->operand[j]; j++) {
				strcpy_s(extref_list[extref_count++], MAX_LINE_LENGTH, t->operand[j]);
			}
			// R 레코드 생성
			if (extref_count > 0) {
				object_code rrec = { 'R', -1, -1, "" };
				for (int k = 0; k < extref_count; k++) {
					char buffer[7];
					snprintf(buffer, sizeof(buffer), "%-6s", extref_list[k]);   // 이름: 6자 공백 패딩
					strcat_s(rrec.object_code, sizeof(rrec.object_code), buffer);
				}
				obj_codes[obj_count++] = rrec;
			}
			continue;
		}

		// CSECT 처리
		else if (strcmp(t->oper, "CSECT") == 0) {

			start_symtab = csect_start_symbol_num[csect_num++];	// 다음 label부터 시작
			// 이전 T 레코드 종료
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;	// 객체 코드 저장

			}

			text_length = 0;	// 텍스트 레코드 길이 초기화
			text_start = -1;	// 텍스트 레코드 시작 주소 초기화
			memset(text_buffer, 0, sizeof(text_buffer));	// 버퍼 초기화
			// 이전 헤더 레코드 길이 업데이트
			if (last_head_idx != -1 ) {
				int end_addr = get_last_allocated_addr(i);
				if (end_addr >= 0) {
					obj_codes[last_head_idx].length = end_addr - obj_codes[last_head_idx].locctr;
				}
			}
			// Modify 레코드 추가
			int sect_start = csect_start_token_line[csect_num - 1];
			int sect_end = csect_start_token_line[csect_num] != 0 ? csect_start_token_line[csect_num] : token_line;
			for (int j = sect_start; j < sect_end; j++) {
				token* tk = token_table[j];
				// 4형식인 경우
				if (tk->oper != NULL && tk->oper[0] == '+') {
					// EXTREF 안에 있는 operand만 수정 대상
					for (int r = 0; r < extref_count; r++) {
						if (tk->operand[0] && strcmp(tk->operand[0], extref_list[r]) == 0) {
							object_code mrec = { 'M', tk->addr + 1, 5, "" }; // 주소 +1, 길이 5
							snprintf(mrec.object_code, sizeof(mrec.object_code), "05+%s", extref_list[r]);
							obj_codes[obj_count++] = mrec;
						}
					}
				}
				// WORD 명령어에서 SYM1 - SYM2 형식 확인
				else if (tk->oper != NULL && strcmp(tk->oper, "WORD") == 0) {
					//-여부 확인
					char* minus = strchr(tk->operand[0], '-');
					if (minus) {
						char left[MAX_LINE_LENGTH], right[MAX_LINE_LENGTH];
						strncpy_s(left, sizeof(left), tk->operand[0], minus - tk->operand[0]);
						strcpy_s(right, sizeof(right), minus + 1);

						for (int r = 0; r < extref_count; r++) {
							if (strcmp(left, extref_list[r]) == 0) {
								object_code mrec = { 'M', tk->addr, 6, "" };
								sprintf_s(mrec.object_code, sizeof(mrec.object_code), "06+%s", extref_list[r]);
								obj_codes[obj_count++] = mrec;
							}
						}
						for (int r = 0; r < extref_count; r++) {
							if (strcmp(right, extref_list[r]) == 0) {
								object_code mrec = { 'M', tk->addr, 6, "" };
								sprintf_s(mrec.object_code, sizeof(mrec.object_code), "06-%s", extref_list[r]);
								obj_codes[obj_count++] = mrec;
							}
						}
					}
				}

			}

			// END 레코드 추가
			if (mainroutine) {
				object_code end_rec = { 'E', -1, -1, "" };
				end_rec.locctr = start_addr;	// 시작 주소
				obj_codes[obj_count++] = end_rec;	// 객체 코드 저장
			}
			else {
				object_code end_rec = { 'E', -1, -1, "" };
				obj_codes[obj_count++] = end_rec;	// 객체 코드 저장
			}
			mainroutine = false;	// 메인루틴 종료

			// 새로운 H 레코드 추가
			object_code new_rec = { 'H', t->addr, 0, "" };
			memset(new_rec.object_code, ' ', 6);  // 6칸 전부 공백
			if (t->label != NULL) {
				size_t len = strlen(t->label);
				if (len > 6) len = 6;
				memcpy(new_rec.object_code, t->label, len);  // 앞에서부터 복사
			}

			new_rec.object_code[6] = '\0';	// 문자열 종료
			new_rec.length = 0;			// 나중에 다시 채움
			last_head_idx = obj_count;	// 헤더 레코드 인덱스 저장
			obj_codes[obj_count++] = new_rec;	// 객체 코드 저장

			//  ext 리스트 초기화
			for (int k = 0; k < 10; k++) {
				memset(extdef_list[k],'\0', MAX_LINE_LENGTH);
				memset(extref_list[k], '\0', MAX_LINE_LENGTH);
			}
			extdef_count = 0;
			extref_count = 0;
			continue;	
		}

		// END 처리
		else if (strcmp(t->oper, "END") == 0) {
			csect_num++;
			//리터럴 처리 (끊지 않고 text_buffer에 그대로 붙이기)
			for (int j = 0; j < literal_num; j++) {
				if (literal_line_index[j] <= i && literal_table[j].addr != -1) {
					int len = 0;
					char obj[70] = "";

					// =C'...' 처리
					if (literal_table[j].symbol[1] == 'C') {
						char* lit = literal_table[j].symbol + 3;  // =C'EOF' → 'EOF'
						for (int k = 0; lit[k] != '\'' && lit[k] != '\0'; k++) {
							char hex[3];
							sprintf_s(hex, sizeof(hex), "%02X", lit[k]);
							strcat_s(obj, sizeof(obj), hex);
						}
						len = strlen(obj) / 2;
					}
					// =X'...' 처리
					else if (literal_table[j].symbol[1] == 'X') {
						strncpy_s(obj, sizeof(obj), literal_table[j].symbol + 3, strlen(literal_table[j].symbol) - 4);
						obj[strlen(literal_table[j].symbol) - 4] = '\0';
						len = strlen(obj) / 2;
					}

					// 텍스트 레코드 초과시 끊기
					if (text_length + len > 30) {
						object_code rec = { 'T', text_start, text_length, "" };
						strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
						obj_codes[obj_count++] = rec;

						text_start = -1;
						text_length = 0;
						j--;
						memset(text_buffer, 0, sizeof(text_buffer));
						continue;
					}

					// 텍스트 시작 주소 설정
					if (text_start == -1)
						text_start = literal_table[j].addr;

					strcat_s(text_buffer, sizeof(text_buffer), obj);
					text_length += len;
				}
			}

			//남은 text_buffer 한 번에 저장
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;

				text_length = 0;
				text_start = -1;
				memset(text_buffer, 0, sizeof(text_buffer));
			}

			//헤더 레코드 길이 설정
			if (last_head_idx != -1) {
				int end_addr = get_last_allocated_addr(i);
				if (end_addr >= 0) {
					obj_codes[last_head_idx].length = end_addr - obj_codes[last_head_idx].locctr;
				}
			}

			int sect_start = csect_start_token_line[csect_num - 1];
			int sect_end = csect_start_token_line[csect_num] != 0 ? csect_start_token_line[csect_num] : token_line;
			// Modify 레코드 추가
			for (int j = sect_start; j < sect_end; j++) {
				token* tk = token_table[j];
				// 4형식인 경우 5비트 수정
				if (tk->oper != NULL && tk->oper[0] == '+') {

					// EXTREF 안에 있는 operand만 수정 대상
					for (int r = 0; r < extref_count; r++) {
						if (tk->operand[0] && strcmp(tk->operand[0], extref_list[r]) == 0) {
							object_code mrec = { 'M', tk->addr + 1, 5, "" }; // 주소 +1, 길이 5
							snprintf(mrec.object_code, sizeof(mrec.object_code), "05+%s", extref_list[r]);
							obj_codes[obj_count++] = mrec;
						}
					}
				}
				// WORD 명령어에서 SYM1 - SYM2 형식 확인
				else if (tk->oper != NULL && strcmp(tk->oper, "WORD") == 0) {
					//-여부 확인
					char* minus = strchr(tk->operand[0], '-');
					if (minus) {
						char left[MAX_LINE_LENGTH], right[MAX_LINE_LENGTH];
						strncpy_s(left, sizeof(left), tk->operand[0], minus - tk->operand[0]);
						strcpy_s(right, sizeof(right), minus + 1);

						for (int r = 0; r < extref_count; r++) {
							if (strcmp(right, extref_list[r]) == 0) {
								object_code mrec = { 'M', tk->addr, 6, "" };
								sprintf_s(mrec.object_code, sizeof(mrec.object_code), "06-%s", extref_list[r]);
								obj_codes[obj_count++] = mrec;
							}
						}
						for (int r = 0; r < extref_count; r++) {
							if (strcmp(left, extref_list[r]) == 0) {
								object_code mrec = { 'M', tk->addr, 6, "" };
								sprintf_s(mrec.object_code, sizeof(mrec.object_code), "06+%s", extref_list[r]);
								obj_codes[obj_count++] = mrec;
							}
						}
					}
				}

			}

			//E 레코드 추가
			object_code end_rec = { 'E', -1, -1, "" };
			end_rec.locctr = mainroutine ? start_addr : -1;
			obj_codes[obj_count++] = end_rec;

			mainroutine = false;
			continue;
		}

		// RESB, RESW 처리 :얘들을 만나면 obj코드 개행을 해야함
		else if (strcmp(t->oper, "RESB") == 0 || strcmp(t->oper, "RESW") == 0) {
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;	// 객체 코드 저장

				text_length = 0;	// 텍스트 레코드 길이 초기화
				text_start = -1;	// 텍스트 레코드 시작 주소 초기화
				memset(text_buffer, 0, sizeof(text_buffer));	// 버퍼 초기화
			}
			continue;
		}

		else if (strcmp(t->oper, "LTORG") == 0) {
			for (int j = 0; j < literal_num; j++) {
				// LTORG 이전에 등장한 리터럴 중 아직 처리 안 한 것만
				if (literal_line_index[j] <= i && literal_table[j].addr != -1) {
					int len = 0;
					char obj[70] = "";

					if (literal_table[j].symbol[1] == 'C') {
						char* lit = literal_table[j].symbol + 3;  // =C'EOF' → 'EOF'
						for (int k = 0; lit[k] != '\'' && lit[k] != '\0'; k++) {
							char hex[3];
							sprintf_s(hex, sizeof(hex), "%02X", lit[k]);
							strcat_s(obj, sizeof(obj), hex);
						}
						len = strlen(obj) / 2;
					}
					else if (literal_table[j].symbol[1] == 'X') {
						strncpy_s(obj, sizeof(obj), literal_table[j].symbol + 3, strlen(literal_table[j].symbol) - 4);
						obj[strlen(literal_table[j].symbol) - 4] = '\0';
						len = strlen(obj) / 2;
					}

					// 텍스트 시작점 초기화
					if (text_start == -1) text_start = literal_table[j].addr;

					// text 길이 초과 시 끊기
					if (text_length + len > 30) {
						object_code rec = { 'T', text_start, text_length, "" };
						strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
						obj_codes[obj_count++] = rec;

						text_start = -1;
						text_length = 0;
						memset(text_buffer, 0, sizeof(text_buffer));

						// 다시 처리
						j--;
						continue;
					}
					// 리터럴 주소를 -1로 설정하여 처리 완료 표시
					literal_table[j].addr = -1;
					strcat_s(text_buffer, sizeof(text_buffer), obj);
					text_length += len;
				}
			}
			continue;
		}


		// BYTE처리
		else if (strcmp(t->oper, "BYTE") == 0) {

			if (text_start == -1) text_start = t->addr;	// 텍스트 레코드 시작 주소 저장

			if (t->operand[0][0] == 'X') {
				strncpy_s(obj, sizeof(obj), t->operand[0] + 2, strlen(t->operand[0]) - 3);	// X'12' -> 12
			}
			else if (t->operand[0][0] == 'C') {
				int len = strlen(t->operand[0]);
				for (int j = 2; j < len - 1; j++) {
					char hex[3] = "";
					sprintf_s(hex, sizeof(hex), "%02X", t->operand[0][j]);
					strcat_s(obj, sizeof(obj), hex);	// C'abc' -> 616263
				}
			}

			if (text_length + (strlen(obj) / 2) > 30) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;	// 객체 코드 저장
				text_length = 0;	// 텍스트 레코드 길이 초기화
				text_start = -1;	// 텍스트 레코드 시작 주소 초기화
				i--;				// 다시 읽기
				memset(text_buffer, 0, sizeof(text_buffer));	// 버퍼 초기화
			}
		}

		//WORD 처리			
		else if (strcmp(t->oper, "WORD") == 0) {
			int value = 0;
			bool is_external = false;	// 외부 심볼 여부

			if (t->operand[0] != NULL) {
				if (get_symval(start_symtab, t->operand[0]) == -1) {
					// 심볼이 없는 경우
					is_external = true;
					value = 0;			// 심볼을 찾지 못한 경우 0으로 초기화
				}
				else {
					// 심볼이 있는 경우
					value = get_symval(start_symtab, t->operand[0]);	// 심볼 주소 가져오기
				}
			}
			sprintf_s(obj, sizeof(obj), "%06X", value);

			// 텍스트 레코드 길이 넘는지 체크
			if (text_length + 3 > 30) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;
				text_start = -1;
				i--;	// 다시 읽기
				text_length = 0;
				memset(text_buffer, 0, sizeof(text_buffer));
			}
		}


		// 일반 명령어
		else{
			if (text_start == -1) {
				text_start = t->addr;	// 텍스트 레코드 시작 주소 저장
			}
			// opcode 검색
			int idx = search_opcode(t->oper);
			if (idx < 0) continue;							// opcode가 유효하지 않음
			unsigned char opcode = inst_table[idx]->op;		// opcode 가져오기
			int format = inst_table[idx]->format;			// 포맷 가져오기
			if (t->oper[0] == '+') format++;				// 4형식인 경우

			// 넘치는지 체크
			if (text_length + format > 30) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;

				text_length = 0;
				text_start = t->addr;
				memset(text_buffer, 0, sizeof(text_buffer));
				i--;
				continue;
			}


			//nixbpe 비트 설정
			set_nixbpe(t, format);


			// object code 생성
			int disp = 0; int target = 0;
			// immediate
			if (t->operand[0] != NULL && t->operand[0][0] == '#') {
				target = (int)strtol(t->operand[0] + 1, NULL, 16);	// immediate 값 가져오기
				unsigned int tmp = (opcode << 16);
				tmp |= (t->nixbpe) << 12;
				tmp |= (target & 0x0FFF);	// 12비트 상대 주소
				sprintf_s(obj, sizeof(obj), "%06X", tmp);
			}
			else {
				// 타겟주소 불러오기
				// literal
				if (t->operand[0] != NULL && t->operand[0][0] == '=') {
					for (int i = 0; i < literal_num; i++) {
						if (strcmp(literal_table[i].symbol, t->operand[0]) == 0) {
							target = literal_table[i].addr;	// 리터럴 주소 가져오기
							break;
						}
					}
				}
				// indirect addressing
				else if (t->operand[0] != NULL && t->operand[0][0] == '@') {
					target = get_symval(start_symtab, t->operand[0] + 1);	// 심볼 주소 가져오기
				}
				// 일반 명령어
				else if (t->operand[0] != NULL) {
					target = get_symval(start_symtab, t->operand[0]);					// 심볼 주소 가져오기	
				}
				int idx = search_opcode(t->oper);
				if (idx < 0) continue;							// opcode가 유효하지 않음
				// 상대 주소 계산
				//피연산자 0개 (ex) RSUB))
				if (inst_table[idx]->ops == 0) {
					unsigned int tmp = (opcode << 16);
					tmp |= (t->nixbpe) << 12;
					sprintf_s(obj, sizeof(obj), "%06X", tmp);
				}
				// format 3
				else if (format == 3) {
					disp = target - (t->addr + 3);	// 상대 주소 계산
					unsigned int tmp = (opcode << 16);
					tmp |= (t->nixbpe) << 12;
					tmp |= (disp & 0x0FFF);	// 12비트 상대 주소
					sprintf_s(obj, sizeof(obj), "%06X", tmp);
				}
				// format 4
				else if (format == 4) {
					disp = target - (t->addr + 4);	// 상대 주소 계산
					unsigned int tmp = (opcode << 24);
					tmp |= (t->nixbpe) << 20;
					tmp |= 0x00000000;
					sprintf_s(obj, sizeof(obj), "%08X", tmp);
				}
				// format 2
				else if (format == 2) {
					unsigned int tmp = opcode << 8;
					if (t->operand[0] != NULL) {
						if (strcmp(t->operand[0], "A") == 0) {
							tmp |= 0x00;
						}
						else if (strcmp(t->operand[0], "X") == 0) {
							tmp |= 0x10;
						}
						else if (strcmp(t->operand[0], "L") == 0) {
							tmp |= 0x20;
						}
						else if (strcmp(t->operand[0], "B") == 0) {
							tmp |= 0x30;
						}
						else if (strcmp(t->operand[0], "S") == 0) {
							tmp |= 0x40;
						}
						else if (strcmp(t->operand[0], "T") == 0) {
							tmp |= 0x50;
						}
					}
					if (t->operand[1] != NULL) {
						if (strcmp(t->operand[1], "A") == 0) {
							tmp |= 0x00;
						}
						else if (strcmp(t->operand[1], "X") == 0) {
							tmp |= 0x01;
						}
						else if (strcmp(t->operand[1], "L") == 0) {
							tmp |= 0x02;
						}
						else if (strcmp(t->operand[1], "B") == 0) {
							tmp |= 0x03;
						}
						else if (strcmp(t->operand[1], "S") == 0) {
							tmp |= 0x04;
						}
						else if (strcmp(t->operand[1], "T") == 0) {
							tmp |= 0x05;
						}
					}
					sprintf_s(obj, sizeof(obj), "%04X", tmp);
				}
			}
		}
		// 텍스트 레코드에 추가
		strcat_s(text_buffer, sizeof(text_buffer), obj);
		text_length += strlen(obj) / 2;	// 텍스트 레코드 길이 업데이트



		//printf("%s\t%s\t%s\t%s\n", t->label ? t->label : "", t->oper ? t->oper : "", t->operand[0] ? t->operand[0] : "", obj);
		
	}
	return 0;
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
	FILE* file;
	if (file_name == NULL) {
		file = stdout;  // 표준 출력
	}
	else {
		fopen_s(&file, file_name, "w");
	}

	for (int i = 0; i < obj_count; i++) {
		object_code* rec = &obj_codes[i];

		// 레코드 타입에 따라 구분 출력
		// H : 헤더 레코드 
		// 프로그램 명은 object_code에 저장되어 있음
		if (rec->record_type == 'H') {
			fprintf(file, "\nH%06s%06X%06X\n",rec->object_code, rec->locctr, rec->length);
		}
		else if (rec->record_type == 'T') {
			fprintf(file, "T%06X%02X%s\n", rec->locctr, rec->length, rec->object_code);
		}
		else if (rec->record_type == 'M') {
			fprintf(file, "M%06X%s\n", rec->locctr, rec->object_code);
		}
		else if (rec->record_type == 'E') {
			if (rec->locctr == -1) {
				fprintf(file, "E\n");
			}
			else {
				fprintf(file, "E%06X\n", rec->locctr);
			}
		}
		else if (rec->record_type == 'D') {
			fprintf(file, "D%s\n", rec->object_code);
		}
		else if (rec->record_type == 'R') {
			fprintf(file, "R%s\n", rec->object_code);
		}
		else {
			fprintf(file, "ERROR: Unknown record type\n");
		}
	}

	if (file != stdout)
		fclose(file);
}

/* ----------------------------------------------------------------------------------
* 설명 : label의 중복을 확인하고 label을 symtable에 추가하는 함수이다.
* 매계 : 테이블 순회 시작 idx, 추가할 label, 주소
* 반환 : 성공 : 0, 실패 : -1
* -----------------------------------------------------------------------------------
*/
int insert_symtab(int start_symtable,  const char* label, int addr) {
	for (int i = start_symtable; i < label_num ; i++) {
		if (strcmp(sym_table[i].symbol, label) == 0) {
			printf("중복된 label이 존재합니다.\n");
			return -1;
		}
	}
	strcpy_s(sym_table[label_num].symbol, sizeof(sym_table[label_num].symbol), label);
	sym_table[label_num].addr = addr;
	label_num++;
	return 0;
}
/* ----------------------------------------------------------------------------------
* 설명 : literal의 중복을 확인하고 literal을 literal_table에 추가하는 함수이다.
* 매계 : 추가할 literal, 주소
* 반환 : 성공 : 0, 실패 : -1
* -----------------------------------------------------------------------------------
*/
int insert_literaltab(const char* label) {
	for (int i = 0; i < literal_num; i++) {
		if (strcmp(literal_table[i].symbol, label) == 0) {
			return -1;
		}
	}
	strcpy_s(literal_table[literal_num].symbol,sizeof(literal_table[literal_num].symbol), label);
	literal_table[literal_num].addr = -1;	// 주소는 아직 미정(-1)

	// literal 등장 시점 저장
	literal_line_index[literal_num] = token_line - 1;  // 현재 줄 인덱스 (token_table 기준)

	literal_num++;
	return 0;	
}

/* ----------------------------------------------------------------------------------
* 설명 : label에 대한 주소를 반환하는 함수이다.
* 매계 : 검색 시작 idx, 가져올 label
* 반환 : 성공 : 주소, 실패 :-1)
* -----------------------------------------------------------------------------------
*/
int get_symval(int start_symtable, const char* label) {
	if (start_symtable > label_num) {
		return -1;
	}

	for (int i = start_symtable; i < label_num; i++) {
		if (strcmp(sym_table[i].symbol, label) == 0) {
			return sym_table[i].addr;
		}
	}
	return -1;
}



/* ----------------------------------------------------------------------------------
* 설명 : LTORG 명령어를 처리하기 위한 함수이다.
* -----------------------------------------------------------------------------------
*/
void assign_littab_addresses() {
	for (int i = 0; i < literal_num; i++) {
		if (literal_table[i].addr == -1) {
			literal_table[i].addr = locctr;
			if (literal_table[i].symbol[1] == 'C') {
				locctr += strlen(literal_table[i].symbol) - 4;
			}
			else if (literal_table[i].symbol[1] == 'X') {
				locctr += (strlen(literal_table[i].symbol) - 4) / 2;
			}
		}
	}
}

/* ----------------------------------------------------------------------------------
* 설명 : nixbpe를 설정 
* 매계 : 토큰, 포맷
* -----------------------------------------------------------------------------------
*/
void set_nixbpe( token* t, int format) {
	t->nixbpe = 0;
	if (format == 2) return;
	if (format == 4) {
		t->nixbpe |= 0x01; // e = 1
	}

	// operand[0] 기준
	if (t->operand[0] != NULL) {
		if (t->operand[0][0] == '#') {
			t->nixbpe |= (1 << 4);  // i = 1
		}
		else if (t->operand[0][0] == '@') {
			t->nixbpe |= (1 << 5);  // n = 1
		}
		else {
			t->nixbpe |= (1 << 5);  // n = 1
			t->nixbpe |= (1 << 4);  // i = 1
		}
	}
	else {
		// operand 없으면 기본으로 ni = 11
		t->nixbpe |= (1 << 5) | (1 << 4);
	}

	// x 사용
	if (t->operand[1] != NULL && strcmp(t->operand[1], "X") == 0) {
		t->nixbpe |= (1 << 3);  // x = 1
	}
	
	// p, b 설정은 format 3/4일 때만 해당
		int target = 0;
		int disp = 0;

	if (t->operand[0] != NULL && t->operand[0][0] != '#') {
		if (format == 3) {
			disp = target - (t->addr + 3);
			if (disp >= -2048 && disp < 2048) {
				t->nixbpe |= (1 << 1);  // p = 1
			}
			else {
				t->nixbpe |= (1 << 2);  // b = 1 (추후 base 처리 필요)
			}
		}
	}
	
}

/* ----------------------------------------------------------------------------------
* 설명 : 마지막 할당된 주소를 찾는 함수이다. SECT가 종료후 주소를 찾기 위해 사용된다.
* 매계 : csect 시작 인덱스
* 반환 : 주소
* -----------------------------------------------------------------------------------
*/
int get_last_allocated_addr(int idx) {
	int sect_start = 0;

	// idx가 몇번째 sect에 있는지 확인
	for (int i = 1; i < csect_num; i++) {
		if (csect_start_token_line[i] >= idx) break;
		sect_start = csect_start_token_line[i];
	}

	int max_addr = -1;

	// 일반 명령어 기준 가장 마지막 주소
	for (int j = idx - 1; j >= sect_start; j--) {
		token* t = token_table[j];
		if (!t || t->addr < 0 || !t->oper) continue;

		if (strcmp(t->oper, "EQU") == 0) continue;

		if (strcmp(t->oper, "RESB") == 0 && t->operand[0])
			max_addr = t->addr + atoi(t->operand[0]);
		else if (strcmp(t->oper, "RESW") == 0 && t->operand[0])
			max_addr = t->addr + 3 * atoi(t->operand[0]);
		else if (strcmp(t->oper, "WORD") == 0)
			max_addr = t->addr + 3;
		else if (strcmp(t->oper, "BYTE") == 0) {
			if (t->operand[0][0] == 'C')
				max_addr = t->addr + strlen(t->operand[0]) - 3;
			else if (t->operand[0][0] == 'X')
				max_addr = t->addr + (strlen(t->operand[0]) - 3) / 2;
		}
		else {
			int inst_idx = search_opcode(t->oper);
			if (inst_idx >= 0) {
				int format = inst_table[inst_idx]->format;
				if (t->oper[0] == '+') format++;
				max_addr = t->addr + format;
			}
		}

		if (max_addr != -1) break;  // 가장 가까운 거 하나만 찾으면 되니까
	}

	// 리터럴 처리 (해당 섹터 안에 있는 것만)
	for (int i = 0; i < literal_num; i++) {
		if (literal_line_index[i] < sect_start || literal_line_index[i] >= idx) continue;
		if (literal_table[i].addr == -1) continue;

		int len = 0;
		if (literal_table[i].symbol[1] == 'C')
			len = strlen(literal_table[i].symbol) - 4;
		else if (literal_table[i].symbol[1] == 'X')
			len = (strlen(literal_table[i].symbol) - 4) / 2;

		int lit_end = literal_table[i].addr + len;
		if (lit_end > max_addr) max_addr = lit_end;
	}

	return max_addr;
}


