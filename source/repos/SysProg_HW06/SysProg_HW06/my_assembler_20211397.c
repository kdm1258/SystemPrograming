/*
 * ���ϸ� : my_assembler.c
 * ��  �� : �� ���α׷��� SIC/XE �ӽ��� ���� ������ Assembler ���α׷��� ���η�ƾ����,
 * �Էµ� ������ �ڵ� ��, ��ɾ �ش��ϴ� OPCODE�� ã�� ����Ѵ�.
 *
 */

 /*
  *
  * ���α׷��� ����� �����Ѵ�.
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
  // ���ϸ��� "00000000"�� �ڽ��� �й����� ������ ��.
#include "my_assembler_20211397.h"

/* -----------------------------------------------------------------------------------
 * ���� : ����ڷ� ���� ����� ������ �޾Ƽ� ��ɾ��� OPCODE�� ã�� ����Ѵ�.
 * �Ű� : ���� ����, ����� ����
 * ��ȯ : ���� = 0, ���� = < 0
 * ���� : ���� ����� ���α׷��� ����Ʈ ������ �����ϴ� ��ƾ�� ������ �ʾҴ�.
 *		   ���� �߰������� �������� �ʴ´�.
 * -----------------------------------------------------------------------------------
 */


int main(int args, char* arg[])
{
	if (init_my_assembler() < 0)
	{
		printf("init_my_assembler: ���α׷� �ʱ�ȭ�� ���� �߽��ϴ�.\n");
		return -1;
	}

	if (assem_pass1() < 0)
	{
		printf("assem_pass1: �н�1 �������� �����Ͽ����ϴ�.  \n");
		return -1;
	}

	//make_opcode_output(NULL);	// ���� �ڵ� ���

	make_symtab_output(NULL);         //"output_symtab.txt"
	make_literaltab_output(NULL);     //  "output_littab.txt"

	if (assem_pass2() < 0)
	{
		printf(" assem_pass2: �н�2 �������� �����Ͽ����ϴ�.  \n");
		return -1;
	}

	make_objectcode_output(NULL);

	return 0;
}

/* -----------------------------------------------------------------------------------
 * ���� : ���α׷� �ʱ�ȭ�� ���� �ڷᱸ�� ���� �� ������ �д� �Լ��̴�.
 * �Ű� : ����
 * ��ȯ : �������� = 0 , ���� �߻� = -1
 * ���� : ������ ��ɾ� ���̺��� ���ο� �������� �ʰ� ������ �����ϰ� �ϱ�
 *		   ���ؼ� ���� ������ �����Ͽ� ���α׷� �ʱ�ȭ�� ���� ������ �о� �� �� �ֵ���
 *		   �����Ͽ���.
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
 * ���� : �ӽ��� ���� ��� �ڵ��� ����(inst_table.txt)�� �о�
 *       ���� ��� ���̺�(inst_table)�� �����ϴ� �Լ��̴�.
 *
 *
 * �Ű� : ���� ��� ����
 * ��ȯ : �������� = 0 , ���� < 0
 * ���� : ���� ������� ������ �����Ӱ� �����Ѵ�. ���ô� ������ ����.
 *
 *	===============================================================================
 *		   | �̸� | ���� | ���� �ڵ� | ���۷����� ���� | \n |
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
		printf("���� ���� ����\n");
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	while (fgets(buffer, sizeof(buffer), file) != NULL)
	{
		inst* new_inst = (inst*)malloc(sizeof(inst));
		if (new_inst == NULL)
		{
			printf("�޸� �Ҵ� ����\n");
			return -1;
		}

		unsigned int op_tmp;
		char format[10];
		
		if (sscanf_s(buffer, "%s\t%s\t%x\t%d",
			new_inst->str, (unsigned int)sizeof(new_inst->str),
			format, (unsigned int)sizeof(format),
			&op_tmp,
			&new_inst->ops) != 4) {
			printf("inst_table.txt ������ �Ľ��ϴµ� �����߽��ϴ�.\n");
			return -1;
		}
		//opcode ����
		new_inst->op = (unsigned char)op_tmp;

	   /* 
	    * format ���� :
		* format == "3/4" �̸� �ϴ� ops == 3�� �����ϰ�, Input_data���� + �� ������ ops == 4�� ������Ʈ �Ѵ�.
		*/
		if(format[1] == '/'){
			new_inst->format = 3;
		}
		else {
			new_inst->format = format[0] - '0';
		}

		// inst_table�� ����
		inst_table[inst_index++] = new_inst;
	}
	fclose(file);
	return errno;
}

/* ----------------------------------------------------------------------------------
 * ���� : ����� �� �ҽ��ڵ� ����(input.txt)�� �о� �ҽ��ڵ� ���̺�(input_data)�� �����ϴ� �Լ��̴�.
 * �Ű� : ������� �ҽ����ϸ�
 * ��ȯ : �������� = 0 , ���� < 0
 * ���� : ���δ����� �����Ѵ�.
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
		printf("���� ���� ����\n");
		return -1;
	}

	fseek(file, 0, SEEK_SET);
	// ������ ���� ������ �о� input_data�� ����
	// MAXLINES�� ���� ��� �ߴ�
	while (fgets(buffer, sizeof(buffer), file) != NULL && line_num < MAX_LINES)
	{
		char* temp = _strdup(buffer);
		if (temp == NULL) {
			printf("�޸� �Ҵ� ���� at line %d\n", line_num);
			break;
		}
		input_data[line_num++] = temp;
	}
	// MAXLINES�� ���� ��� ����
	if (line_num >= MAX_LINES) {
		printf("�ִ� ���μ��� �ʰ��Ͽ����ϴ�.\n");
		return -1;
	}
	fclose(file);
	return errno;
}

/* ----------------------------------------------------------------------------------
 * ���� : �ҽ� �ڵ带 �о�� ��ū������ �м��ϰ� ��ū ���̺��� �ۼ��ϴ� �Լ��̴�.
 *        �н� 1�� ���� ȣ��ȴ�.
 * �Ű� : �Ľ��� ���ϴ� ���ڿ�
 * ��ȯ : �������� = 0 , ���� < 0
 * ���� : my_assembler ���α׷������� ���δ����� ��ū �� ������Ʈ ������ �ϰ� �ִ�.
 * ----------------------------------------------------------------------------------
 */
int token_parsing(char* str)
{
	// �ӽ���ū �ʱ�ȭ
	token* tok = (token*)malloc(sizeof(token));
	if (tok == NULL) {
		printf("�޸� �Ҵ� ����\n");
		return -1;
	}
	tok->label = NULL;
	tok->oper = NULL;
	for (int i = 0; i < MAX_OPERAND; i++)	tok->operand[i] = NULL;
	tok->comment[0] = '\0';

	// �ּ�ó��
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
	char* context = NULL;	// strtok_s()�� context ����
	int num_operands = MAX_OPERAND;	// �ǿ����� ����


	// label�� ���°�� : '\t'�� ����
	if (str[0] == '\t') {
		tmp_str = tmp_str + 1;
		tmp_oper = strtok_s(tmp_str, " \t\n", &context);
	}
	//label�� �ִ� ���
	else {
		tmp_label = strtok_s(tmp_str, " \t\n", &context);
		tmp_oper = strtok_s(NULL, " \t\n", &context);
	}

	if (tmp_oper != NULL) {
		tok->oper = _strdup(tmp_oper);

		// operator�� NULL�� �ƴѰ�� �ִ� �ǿ����� ������ �����´�
		for (int i = 0; i < inst_index; i++) {
			const char* opcheck = (tmp_oper[0] == '+') ? tmp_oper + 1 : tmp_oper;
			if (strcmp(inst_table[i]->str, opcheck) == 0) {
				num_operands = inst_table[i]->ops;
				break;
			}
		}
	}
	
	// operand ������ 0 ex) RSUB�� ��쿡�� ������ ���ڿ��� ��� commentó���Ѵ�.
	if (num_operands > 0)	tmp_operands = strtok_s(NULL, " \t\n", &context);
	tmp_comment = strtok_s(NULL, "", &context);	//���� ���ڿ��� ��� �ּ����� ó��
	// label�� ����� �Ҵ�
	if (tmp_label != NULL) {
		tok->label = _strdup(tmp_label);
	}

   /* operand parsing :
	* operand�� ','�� ���еǾ� �ִ�.
	* tmp_operands�� ���� ������ ������ comment�� ','�� �����ϱ� ���ؼ��̴�.
	*/
	if (tmp_operands != NULL) {
		char* operand_context = NULL;	// strtok_s()�� context ����
		char* tmp_operand = strtok_s(tmp_operands, ",", &operand_context);
		int i = 0;
		while (tmp_operand != NULL && i < MAX_OPERAND) {
			tok->operand[i++] = _strdup(tmp_operand);
			tmp_operand = strtok_s(NULL, ",", &operand_context);
		}
	}

	// comment�� ','�� ������� ��°�� ����
	if (tmp_comment != NULL) {
		//comment ��������
		while (*tmp_comment == ' ' || *tmp_comment == '\t') tmp_comment++;	
		strcpy_s(tok->comment,sizeof(tok->comment), tmp_comment);
	}

	token_table[token_line++] = tok;
	return 0;
}

/* ----------------------------------------------------------------------------------
* ���� : ����� �ڵ带 ���� �н�1������ �����ϴ� �Լ��̴�.
*		   �н�1������..
*		   1. ���α׷� �ҽ��� ��ĵ�Ͽ� �ش��ϴ� ��ū������ �и��Ͽ� ���α׷� ���κ� ��ū
*		   ���̺��� �����Ѵ�.
*          2. ��ū ���̺��� token_parsing()�� ȣ���Ͽ� �����Ѵ�.
*          3. assem_pass2 �������� ����ϱ� ���� �ɺ����̺� �� ���ͷ� ���̺��� �����Ѵ�.
*
*
*
* �Ű� : ����
* ��ȯ : ���� ���� = 0 , ���� = < 0
* ���� : ���� �ʱ� ���������� ������ ���� �˻縦 ���� �ʰ� �Ѿ �����̴�.
*	     ���� ������ ���� �˻� ��ƾ�� �߰��ؾ� �Ѵ�.
*
*        OPCODE ��� ���α׷������� �ɺ����̺�, ���ͷ����̺��� �������� �ʾƵ� �ȴ�.
*        �׷���, ���� ������Ʈ 1�� �����ϱ� ���ؼ��� �ɺ����̺�, ���ͷ����̺��� �ʿ��ϴ�.
*
* -----------------------------------------------------------------------------------
*/
static int assem_pass1(void)
{
	csect_start_token_line[0] = 0;	// CSECT ���� ��ū ���� ����
	bool start_encounter = false;	// START ����
	int start_symtab = 0;	// sym table ��Ī ���� �ε���

	/* input_data�� ���ڿ��� ���پ� �Է� �޾Ƽ�
	 * token_parsing()�� ȣ���Ͽ� _token�� ����
	 */
	for (int i = 0; i < line_num; i++) {
		if (token_parsing(input_data[i]) < 0) {
			printf("token_parsing: ��ū �Ľ̿� �����Ͽ����ϴ�.\n");
			return -1;
		}

		// �Ľ̵� ��ū 
		token* t = token_table[i];
		t->addr = -1; // �ּ� �ʱ�ȭ
		if (!t || !t->oper) continue; // NULL üũ

		//comment line
		if (t->oper == NULL && t->comment[0] == '.') {
			// �ּ� ����
			continue;
		}

		//START
		if (strcmp(t->oper, "START") == 0) {
			// START�� 2ȸ �̻� ������ ����
			if (start_encounter) {
				printf("START ��ɾ �ߺ��Ǿ����ϴ�.\n");
				return -1;
			}

			// ���α׷� ���� �ּҸ� ����
			if (t->operand[0] != NULL) {
				locctr = (int)strtol(t->operand[0], NULL, 16);
			}
			else {
				locctr = 0; // �����ּҰ� ������ locctr�� 0���� �ʱ�ȭ
			}
			start_addr = locctr;	// ���α׷� ���� �ּ� ����
			start_encounter = true;	// START�� �������� ǥ��

			// START label����
			if (t->label != NULL) {
				if (insert_symtab(start_symtab, t->label, locctr) < 0) return -1;
			}
			t->addr = locctr;	// locctr�� addr�� ����
			continue;	// START�� locctr�� ������Ű�� ����
		}

		// CSECT 
		if (t->oper && strcmp(t->oper, "CSECT") == 0) {
			assign_littab_addresses();
			csect_start_symbol_num[csect_num++] = label_num; // CSECT ���� �ɺ� ��ȣ ����
			csect_start_token_line[csect_num] = i; // CSECT ���� ��ū ���� ����
			start_symtab = label_num;	// ���� label���� ����
			insert_symtab(start_symtab, "", NULL); // �� label ���� ���п�
			locctr = 0;
			if (t->label) insert_symtab(start_symtab, t->label, locctr); // CSECT ���� ��ġ ����
			t->addr = locctr;	// locctr�� addr�� ����
			continue;	// LOCCTR ������Ʈ ����
		}

		// label ����
		if (t->label != NULL && strcmp(t->oper, "EQU") != 0 ) {
			if (insert_symtab(start_symtab, t->label, locctr) < 0) return -1;
		}

		// EQU
		if (strcmp(t->oper, "EQU") == 0) {
			int value = 0;

			// �ǿ����ڰ� '*'�� ���
			if (strcmp(t->operand[0], "*") == 0) {
				value = locctr;
			}
			// �ǿ����ڰ� ������ ���
			else if (isdigit(t->operand[0][0])) {
				value = atoi(t->operand[0]);
			}
			// '-'���� ó��
			else if(strchr(t->operand[0], '-') != NULL) {
				char operand_copy[100];
				strcpy_s(operand_copy, sizeof(operand_copy), t->operand[0]);

				char* left = strtok(operand_copy, "-");
				char* right = strtok(NULL, "-");

				int left_val = get_symval(start_symtab , left);
				int right_val = get_symval(start_symtab ,right);

				value = (left_val == -1 || right_val == -1) ? 0 : left_val - right_val;
			}
			// �ǿ����ڰ� �ɺ��� ���
			else {
				int symvalue = get_symval(start_symtab, t->operand[0]);
				value = (symvalue == -1) ? 0 : symvalue;	//�ɺ��� ã�� ���Ѱ�� 0���� ä��
			}

			// �ɺ� ���̺� ����
			if (t->label != NULL) {
				if (insert_symtab(start_symtab, t->label, value) < 0) return -1;
			}
			t->addr = locctr;	// locctr�� addr�� ����
			continue; // LOCCTR ������Ʈ ����
		}

		//LTORG
		if (strcmp(t->oper, "LTORG") == 0) {
			assign_littab_addresses();
			continue;
		}

		// ���ͷ� ó��
		if (t->operand[0] != NULL && t->operand[0][0] == '=') {
			// ���ͷ� ���̺� �߰�
			insert_literaltab(t->operand[0]);
		}

		t->addr = locctr;	// locctr�� addr�� ����
		
		// ���þ� ó��
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
				return -1; // �߸��� BYTE ����
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
		// opcode �˻�
			int idx = search_opcode(t->oper);
			if (idx >= 0) {
				if (t->oper[0] == '+') {
					// 4������ ���
					locctr += inst_table[idx]->format + 1;
				}
				else {
					locctr += inst_table[idx]->format;
				}
			}
		}

	}
	// locctr ������Ʈ
	assign_littab_addresses();
	return 0;
}

/* ----------------------------------------------------------------------------------
 * ���� : �Է� ���ڿ��� ���� �ڵ������� �˻��ϴ� �Լ��̴�.
 * �Ű� : ��ū ������ ���е� ���ڿ�
 * ��ȯ : �������� = ���� ���̺� �ε���, ���� < 0
 * ���� : ���� ��� ���̺��� Ư�� ��� �˻��Ͽ�, �ش� ��� ��ġ�� �ε����� ��ȯ�Ѵ�.
 *        '+JSUB'�� ���� ���ڿ��� ���� ó���� �����Ӱ� ó���Ѵ�.
 *
 * ----------------------------------------------------------------------------------
 */
int search_opcode(char* str)
{
	// operator�� NULL�� ���
	if (str == NULL)
	{
		return -1;
	}
	//4���� ó�� : +�� ����
	if (str[0] == '+')
	{
		str++;	// +�� ����
				

	}
	for (int i = 0; i < inst_index; i++)
	{
		if (strcmp(inst_table[i]->str, str) == 0)
		{
			return i;
		}
	}
	return -1;	// ã�� ���� ���
}

/* ----------------------------------------------------------------------------------
* ���� : �Էµ� ���ڿ��� �̸��� ���� ���Ͽ� ���α׷��� ����� �����ϴ� �Լ��̴�.
*
* �Ű� : ������ ������Ʈ ���ϸ�
* ��ȯ : ����
* ���� : �ҽ��ڵ� ��ɾ� �տ� OPCODE�� ��ϵ� �ڵ带 ���Ͽ� ����Ѵ�.
*        ������ NULL���� ���´ٸ� ���α׷��� ����� stdout���� ������
*        ȭ�鿡 ������ش�.
*
*        OPCODE ��� ���α׷��� ���� output ������ �����ϴ� �Լ��̴�.
*        (���� ������Ʈ 1������ ���ʿ�)
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
			printf("���� ���� ����\n");
			return;
		}
	}

	fseek(file, 0, SEEK_SET);

	for (int i = 0; i < token_line; i++)
	{
		token* t = token_table[i];

		// �ּ� ����
		if (t->oper == NULL)
		{
			fprintf(file, "%s\n", t->comment);
			continue;
		}

		// operand ���ڿ� �����
		char operand_str[100] = "";
		for (int j = 0; j < MAX_OPERAND; j++) {
			if (t->operand[j] != NULL) {
				if (operand_str[0] != '\0') {
					strcat_s(operand_str, sizeof(operand_str), ",");
				}
				strcat_s(operand_str, sizeof(operand_str), t->operand[j]);
			}
		}

		// opcode ���ڿ� ����� 
		char opcode_str[10] = "";
		int idx = search_opcode(t->oper);
		if (idx != -1) {
			sprintf_s(opcode_str, sizeof(opcode_str), "%02X", inst_table[idx]->op);
		}

		// ���ĵ� ���
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
* ���� : �Էµ� ���ڿ��� �̸��� ���� ���Ͽ� ���α׷��� ����� �����ϴ� �Լ��̴�.
*        ���⼭ ��µǴ� ������ SYMBOL�� �ּҰ��� ����� TABLE�̴�.
* �Ű� : ������ ������Ʈ ���ϸ� Ȥ�� ���
* ��ȯ : ����
* ���� : ������ NULL���� ���´ٸ� ���α׷��� ����� stdout���� ������
*        ȭ�鿡 ������ش�.
*
* -----------------------------------------------------------------------------------
*/
void make_symtab_output(char* file_name)
{
	FILE* file;
	if (file_name == NULL)
		file = stdout;
	else
		// ���� ����
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
		fclose(file);	// ���� �ݱ�
}


/* ----------------------------------------------------------------------------------
* ���� : �Էµ� ���ڿ��� �̸��� ���� ���Ͽ� ���α׷��� ����� �����ϴ� �Լ��̴�.
*        ���⼭ ��µǴ� ������ LITERAL�� �ּҰ��� ����� TABLE�̴�.
* �Ű� : ������ ������Ʈ ���ϸ�
* ��ȯ : ����
* ���� : ������ NULL���� ���´ٸ� ���α׷��� ����� stdout���� ������
*        ȭ�鿡 ������ش�.
*
* -----------------------------------------------------------------------------------
*/
void make_literaltab_output(char* filename)
{
	FILE* file;
	if (filename == NULL)
		file = stdout;
	else
		// ���� ����
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
		fclose(file);	// ���� �ݱ�
}


/* -----------------------------------------------------------------------------------
 * ���� : ����� �ڵ带 ���� �ڵ�� �ٲٱ� ���� �н�2 ������ �����ϴ� �Լ��̴�.
 *		   �н� 2������ ���α׷��� ����� �ٲٴ� �۾��� ���� ������ ����ȴ�.
 *		   ������ ���� �۾��� ����Ǿ� ����.
 *		   1. ������ �ش� ����� ��ɾ ����� �ٲٴ� �۾��� �����Ѵ�.
 * �Ű� : ����
 * ��ȯ : �������� = 0, �����߻� = < 0
 * ���� :
 * -----------------------------------------------------------------------------------
 */

static int assem_pass2(void)
{
	csect_num = 0;
	int start_symtab = 0;		// sym table ��Ī ���� �ε���
	bool mainroutine = false;	// ���η�ƾ ����
	int last_head_idx = -1;		// ������ ��� ���ڵ� �ε���
	int text_length = 0;		// �ؽ�Ʈ ���ڵ� ����
	int text_start = -1;		// �ؽ�Ʈ ���ڵ� ���� �ּ�
	char text_buffer[70] = "";	// �ؽ�Ʈ ���ڵ� ����


	// EXTREF, EXTDEF ��� �����
	char extdef_list[10][MAX_LINE_LENGTH]; int extdef_count = 0;
	char extref_list[10][MAX_LINE_LENGTH]; int extref_count = 0;

	for (int i = 0; i < token_line; i++) {
		token* t = token_table[i];
		object_code oc;
		char obj[10] = "";		// ��ü �ڵ� ����

		// �ּ� ����
		if (!t->oper || t->oper[0] == '.') continue;

		// START ó��
		if (t->oper != NULL && strcmp(t->oper, "START") == 0) {
			last_head_idx = obj_count;	// ��� ���ڵ� �ε��� ����
			mainroutine = true;

			// start �ɺ��� H�� ����
			oc.record_type = 'H';
			oc.locctr = t->addr;	// locctr ����

			// object_code�� �������� �ʱ�ȭ�ϰ�, ���α׷� �̸� ����
			memset(oc.object_code, ' ', 6);  // 6ĭ ���� ����
			if (t->label != NULL) {
				size_t len = strlen(t->label);
				if (len > 6) len = 6;
				memcpy(oc.object_code, t->label, len);  // �տ������� ����
			}
			oc.object_code[6] = '\0';
			obj_codes[obj_count++] = oc;	// ��ü �ڵ� ����
			continue;
		}

		// EXTDEF ó�� �� D ���ڵ�
		if (strcmp(t->oper, "EXTDEF") == 0) {
			for (int j = 0; j < MAX_OPERAND && t->operand[j]; j++) {
				strcpy_s(extdef_list[extdef_count++], MAX_LINE_LENGTH, t->operand[j]);
			}
			// EXTDEF�� ������ D ���ڵ� ����
			if (extdef_count > 0) {
				object_code drec = { 'D', -1, -1, "" };
				for (int k = 0; k < extdef_count; k++) {
					int addr = get_symval(start_symtab, extdef_list[k]);
					char name[7], entry[20];
					snprintf(name, sizeof(name), "%-6s", extdef_list[k]);       // �̸�: 6�� ���� �е�
					snprintf(entry, sizeof(entry), "%s%06X", name, addr);       // ��ü: �̸�+�ּ�
					strcat_s(drec.object_code, sizeof(drec.object_code), entry);
				}
				obj_codes[obj_count++] = drec;
			}

			continue;
		}

		// EXTREF ó�� �� R ���ڵ�
		if (strcmp(t->oper, "EXTREF") == 0) {
			for (int j = 0; j < MAX_OPERAND && t->operand[j]; j++) {
				strcpy_s(extref_list[extref_count++], MAX_LINE_LENGTH, t->operand[j]);
			}
			// R ���ڵ� ����
			if (extref_count > 0) {
				object_code rrec = { 'R', -1, -1, "" };
				for (int k = 0; k < extref_count; k++) {
					char buffer[7];
					snprintf(buffer, sizeof(buffer), "%-6s", extref_list[k]);   // �̸�: 6�� ���� �е�
					strcat_s(rrec.object_code, sizeof(rrec.object_code), buffer);
				}
				obj_codes[obj_count++] = rrec;
			}
			continue;
		}

		// CSECT ó��
		else if (strcmp(t->oper, "CSECT") == 0) {

			start_symtab = csect_start_symbol_num[csect_num++];	// ���� label���� ����
			// ���� T ���ڵ� ����
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;	// ��ü �ڵ� ����

			}

			text_length = 0;	// �ؽ�Ʈ ���ڵ� ���� �ʱ�ȭ
			text_start = -1;	// �ؽ�Ʈ ���ڵ� ���� �ּ� �ʱ�ȭ
			memset(text_buffer, 0, sizeof(text_buffer));	// ���� �ʱ�ȭ
			// ���� ��� ���ڵ� ���� ������Ʈ
			if (last_head_idx != -1 ) {
				int end_addr = get_last_allocated_addr(i);
				if (end_addr >= 0) {
					obj_codes[last_head_idx].length = end_addr - obj_codes[last_head_idx].locctr;
				}
			}
			// Modify ���ڵ� �߰�
			int sect_start = csect_start_token_line[csect_num - 1];
			int sect_end = csect_start_token_line[csect_num] != 0 ? csect_start_token_line[csect_num] : token_line;
			for (int j = sect_start; j < sect_end; j++) {
				token* tk = token_table[j];
				// 4������ ���
				if (tk->oper != NULL && tk->oper[0] == '+') {
					// EXTREF �ȿ� �ִ� operand�� ���� ���
					for (int r = 0; r < extref_count; r++) {
						if (tk->operand[0] && strcmp(tk->operand[0], extref_list[r]) == 0) {
							object_code mrec = { 'M', tk->addr + 1, 5, "" }; // �ּ� +1, ���� 5
							snprintf(mrec.object_code, sizeof(mrec.object_code), "05+%s", extref_list[r]);
							obj_codes[obj_count++] = mrec;
						}
					}
				}
				// WORD ��ɾ�� SYM1 - SYM2 ���� Ȯ��
				else if (tk->oper != NULL && strcmp(tk->oper, "WORD") == 0) {
					//-���� Ȯ��
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

			// END ���ڵ� �߰�
			if (mainroutine) {
				object_code end_rec = { 'E', -1, -1, "" };
				end_rec.locctr = start_addr;	// ���� �ּ�
				obj_codes[obj_count++] = end_rec;	// ��ü �ڵ� ����
			}
			else {
				object_code end_rec = { 'E', -1, -1, "" };
				obj_codes[obj_count++] = end_rec;	// ��ü �ڵ� ����
			}
			mainroutine = false;	// ���η�ƾ ����

			// ���ο� H ���ڵ� �߰�
			object_code new_rec = { 'H', t->addr, 0, "" };
			memset(new_rec.object_code, ' ', 6);  // 6ĭ ���� ����
			if (t->label != NULL) {
				size_t len = strlen(t->label);
				if (len > 6) len = 6;
				memcpy(new_rec.object_code, t->label, len);  // �տ������� ����
			}

			new_rec.object_code[6] = '\0';	// ���ڿ� ����
			new_rec.length = 0;			// ���߿� �ٽ� ä��
			last_head_idx = obj_count;	// ��� ���ڵ� �ε��� ����
			obj_codes[obj_count++] = new_rec;	// ��ü �ڵ� ����

			//  ext ����Ʈ �ʱ�ȭ
			for (int k = 0; k < 10; k++) {
				memset(extdef_list[k],'\0', MAX_LINE_LENGTH);
				memset(extref_list[k], '\0', MAX_LINE_LENGTH);
			}
			extdef_count = 0;
			extref_count = 0;
			continue;	
		}

		// END ó��
		else if (strcmp(t->oper, "END") == 0) {
			csect_num++;
			//���ͷ� ó�� (���� �ʰ� text_buffer�� �״�� ���̱�)
			for (int j = 0; j < literal_num; j++) {
				if (literal_line_index[j] <= i && literal_table[j].addr != -1) {
					int len = 0;
					char obj[70] = "";

					// =C'...' ó��
					if (literal_table[j].symbol[1] == 'C') {
						char* lit = literal_table[j].symbol + 3;  // =C'EOF' �� 'EOF'
						for (int k = 0; lit[k] != '\'' && lit[k] != '\0'; k++) {
							char hex[3];
							sprintf_s(hex, sizeof(hex), "%02X", lit[k]);
							strcat_s(obj, sizeof(obj), hex);
						}
						len = strlen(obj) / 2;
					}
					// =X'...' ó��
					else if (literal_table[j].symbol[1] == 'X') {
						strncpy_s(obj, sizeof(obj), literal_table[j].symbol + 3, strlen(literal_table[j].symbol) - 4);
						obj[strlen(literal_table[j].symbol) - 4] = '\0';
						len = strlen(obj) / 2;
					}

					// �ؽ�Ʈ ���ڵ� �ʰ��� ����
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

					// �ؽ�Ʈ ���� �ּ� ����
					if (text_start == -1)
						text_start = literal_table[j].addr;

					strcat_s(text_buffer, sizeof(text_buffer), obj);
					text_length += len;
				}
			}

			//���� text_buffer �� ���� ����
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;

				text_length = 0;
				text_start = -1;
				memset(text_buffer, 0, sizeof(text_buffer));
			}

			//��� ���ڵ� ���� ����
			if (last_head_idx != -1) {
				int end_addr = get_last_allocated_addr(i);
				if (end_addr >= 0) {
					obj_codes[last_head_idx].length = end_addr - obj_codes[last_head_idx].locctr;
				}
			}

			int sect_start = csect_start_token_line[csect_num - 1];
			int sect_end = csect_start_token_line[csect_num] != 0 ? csect_start_token_line[csect_num] : token_line;
			// Modify ���ڵ� �߰�
			for (int j = sect_start; j < sect_end; j++) {
				token* tk = token_table[j];
				// 4������ ��� 5��Ʈ ����
				if (tk->oper != NULL && tk->oper[0] == '+') {

					// EXTREF �ȿ� �ִ� operand�� ���� ���
					for (int r = 0; r < extref_count; r++) {
						if (tk->operand[0] && strcmp(tk->operand[0], extref_list[r]) == 0) {
							object_code mrec = { 'M', tk->addr + 1, 5, "" }; // �ּ� +1, ���� 5
							snprintf(mrec.object_code, sizeof(mrec.object_code), "05+%s", extref_list[r]);
							obj_codes[obj_count++] = mrec;
						}
					}
				}
				// WORD ��ɾ�� SYM1 - SYM2 ���� Ȯ��
				else if (tk->oper != NULL && strcmp(tk->oper, "WORD") == 0) {
					//-���� Ȯ��
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

			//E ���ڵ� �߰�
			object_code end_rec = { 'E', -1, -1, "" };
			end_rec.locctr = mainroutine ? start_addr : -1;
			obj_codes[obj_count++] = end_rec;

			mainroutine = false;
			continue;
		}

		// RESB, RESW ó�� :����� ������ obj�ڵ� ������ �ؾ���
		else if (strcmp(t->oper, "RESB") == 0 || strcmp(t->oper, "RESW") == 0) {
			if (text_length > 0) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;	// ��ü �ڵ� ����

				text_length = 0;	// �ؽ�Ʈ ���ڵ� ���� �ʱ�ȭ
				text_start = -1;	// �ؽ�Ʈ ���ڵ� ���� �ּ� �ʱ�ȭ
				memset(text_buffer, 0, sizeof(text_buffer));	// ���� �ʱ�ȭ
			}
			continue;
		}

		else if (strcmp(t->oper, "LTORG") == 0) {
			for (int j = 0; j < literal_num; j++) {
				// LTORG ������ ������ ���ͷ� �� ���� ó�� �� �� �͸�
				if (literal_line_index[j] <= i && literal_table[j].addr != -1) {
					int len = 0;
					char obj[70] = "";

					if (literal_table[j].symbol[1] == 'C') {
						char* lit = literal_table[j].symbol + 3;  // =C'EOF' �� 'EOF'
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

					// �ؽ�Ʈ ������ �ʱ�ȭ
					if (text_start == -1) text_start = literal_table[j].addr;

					// text ���� �ʰ� �� ����
					if (text_length + len > 30) {
						object_code rec = { 'T', text_start, text_length, "" };
						strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
						obj_codes[obj_count++] = rec;

						text_start = -1;
						text_length = 0;
						memset(text_buffer, 0, sizeof(text_buffer));

						// �ٽ� ó��
						j--;
						continue;
					}
					// ���ͷ� �ּҸ� -1�� �����Ͽ� ó�� �Ϸ� ǥ��
					literal_table[j].addr = -1;
					strcat_s(text_buffer, sizeof(text_buffer), obj);
					text_length += len;
				}
			}
			continue;
		}


		// BYTEó��
		else if (strcmp(t->oper, "BYTE") == 0) {

			if (text_start == -1) text_start = t->addr;	// �ؽ�Ʈ ���ڵ� ���� �ּ� ����

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
				obj_codes[obj_count++] = rec;	// ��ü �ڵ� ����
				text_length = 0;	// �ؽ�Ʈ ���ڵ� ���� �ʱ�ȭ
				text_start = -1;	// �ؽ�Ʈ ���ڵ� ���� �ּ� �ʱ�ȭ
				i--;				// �ٽ� �б�
				memset(text_buffer, 0, sizeof(text_buffer));	// ���� �ʱ�ȭ
			}
		}

		//WORD ó��			
		else if (strcmp(t->oper, "WORD") == 0) {
			int value = 0;
			bool is_external = false;	// �ܺ� �ɺ� ����

			if (t->operand[0] != NULL) {
				if (get_symval(start_symtab, t->operand[0]) == -1) {
					// �ɺ��� ���� ���
					is_external = true;
					value = 0;			// �ɺ��� ã�� ���� ��� 0���� �ʱ�ȭ
				}
				else {
					// �ɺ��� �ִ� ���
					value = get_symval(start_symtab, t->operand[0]);	// �ɺ� �ּ� ��������
				}
			}
			sprintf_s(obj, sizeof(obj), "%06X", value);

			// �ؽ�Ʈ ���ڵ� ���� �Ѵ��� üũ
			if (text_length + 3 > 30) {
				object_code rec = { 'T', text_start, text_length, "" };
				strcpy_s(rec.object_code, sizeof(rec.object_code), text_buffer);
				obj_codes[obj_count++] = rec;
				text_start = -1;
				i--;	// �ٽ� �б�
				text_length = 0;
				memset(text_buffer, 0, sizeof(text_buffer));
			}
		}


		// �Ϲ� ��ɾ�
		else{
			if (text_start == -1) {
				text_start = t->addr;	// �ؽ�Ʈ ���ڵ� ���� �ּ� ����
			}
			// opcode �˻�
			int idx = search_opcode(t->oper);
			if (idx < 0) continue;							// opcode�� ��ȿ���� ����
			unsigned char opcode = inst_table[idx]->op;		// opcode ��������
			int format = inst_table[idx]->format;			// ���� ��������
			if (t->oper[0] == '+') format++;				// 4������ ���

			// ��ġ���� üũ
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


			//nixbpe ��Ʈ ����
			set_nixbpe(t, format);


			// object code ����
			int disp = 0; int target = 0;
			// immediate
			if (t->operand[0] != NULL && t->operand[0][0] == '#') {
				target = (int)strtol(t->operand[0] + 1, NULL, 16);	// immediate �� ��������
				unsigned int tmp = (opcode << 16);
				tmp |= (t->nixbpe) << 12;
				tmp |= (target & 0x0FFF);	// 12��Ʈ ��� �ּ�
				sprintf_s(obj, sizeof(obj), "%06X", tmp);
			}
			else {
				// Ÿ���ּ� �ҷ�����
				// literal
				if (t->operand[0] != NULL && t->operand[0][0] == '=') {
					for (int i = 0; i < literal_num; i++) {
						if (strcmp(literal_table[i].symbol, t->operand[0]) == 0) {
							target = literal_table[i].addr;	// ���ͷ� �ּ� ��������
							break;
						}
					}
				}
				// indirect addressing
				else if (t->operand[0] != NULL && t->operand[0][0] == '@') {
					target = get_symval(start_symtab, t->operand[0] + 1);	// �ɺ� �ּ� ��������
				}
				// �Ϲ� ��ɾ�
				else if (t->operand[0] != NULL) {
					target = get_symval(start_symtab, t->operand[0]);					// �ɺ� �ּ� ��������	
				}
				int idx = search_opcode(t->oper);
				if (idx < 0) continue;							// opcode�� ��ȿ���� ����
				// ��� �ּ� ���
				//�ǿ����� 0�� (ex) RSUB))
				if (inst_table[idx]->ops == 0) {
					unsigned int tmp = (opcode << 16);
					tmp |= (t->nixbpe) << 12;
					sprintf_s(obj, sizeof(obj), "%06X", tmp);
				}
				// format 3
				else if (format == 3) {
					disp = target - (t->addr + 3);	// ��� �ּ� ���
					unsigned int tmp = (opcode << 16);
					tmp |= (t->nixbpe) << 12;
					tmp |= (disp & 0x0FFF);	// 12��Ʈ ��� �ּ�
					sprintf_s(obj, sizeof(obj), "%06X", tmp);
				}
				// format 4
				else if (format == 4) {
					disp = target - (t->addr + 4);	// ��� �ּ� ���
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
		// �ؽ�Ʈ ���ڵ忡 �߰�
		strcat_s(text_buffer, sizeof(text_buffer), obj);
		text_length += strlen(obj) / 2;	// �ؽ�Ʈ ���ڵ� ���� ������Ʈ



		//printf("%s\t%s\t%s\t%s\n", t->label ? t->label : "", t->oper ? t->oper : "", t->operand[0] ? t->operand[0] : "", obj);
		
	}
	return 0;
}

/* ----------------------------------------------------------------------------------
* ���� : �Էµ� ���ڿ��� �̸��� ���� ���Ͽ� ���α׷��� ����� �����ϴ� �Լ��̴�.
*        ���⼭ ��µǴ� ������ object code�̴�.
* �Ű� : ������ ������Ʈ ���ϸ�
* ��ȯ : ����
* ���� : ������ NULL���� ���´ٸ� ���α׷��� ����� stdout���� ������
*        ȭ�鿡 ������ش�.
*        ������ �־��� ��� ����� ������ �����ؾ� �Ѵ�.
*        ���������� �� ���� ������ ���� ���� Ȥ�� ���� ������ ���̴� ����Ѵ�.
*
* -----------------------------------------------------------------------------------
*/
void make_objectcode_output(char* file_name)
{
	FILE* file;
	if (file_name == NULL) {
		file = stdout;  // ǥ�� ���
	}
	else {
		fopen_s(&file, file_name, "w");
	}

	for (int i = 0; i < obj_count; i++) {
		object_code* rec = &obj_codes[i];

		// ���ڵ� Ÿ�Կ� ���� ���� ���
		// H : ��� ���ڵ� 
		// ���α׷� ���� object_code�� ����Ǿ� ����
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
* ���� : label�� �ߺ��� Ȯ���ϰ� label�� symtable�� �߰��ϴ� �Լ��̴�.
* �Ű� : ���̺� ��ȸ ���� idx, �߰��� label, �ּ�
* ��ȯ : ���� : 0, ���� : -1
* -----------------------------------------------------------------------------------
*/
int insert_symtab(int start_symtable,  const char* label, int addr) {
	for (int i = start_symtable; i < label_num ; i++) {
		if (strcmp(sym_table[i].symbol, label) == 0) {
			printf("�ߺ��� label�� �����մϴ�.\n");
			return -1;
		}
	}
	strcpy_s(sym_table[label_num].symbol, sizeof(sym_table[label_num].symbol), label);
	sym_table[label_num].addr = addr;
	label_num++;
	return 0;
}
/* ----------------------------------------------------------------------------------
* ���� : literal�� �ߺ��� Ȯ���ϰ� literal�� literal_table�� �߰��ϴ� �Լ��̴�.
* �Ű� : �߰��� literal, �ּ�
* ��ȯ : ���� : 0, ���� : -1
* -----------------------------------------------------------------------------------
*/
int insert_literaltab(const char* label) {
	for (int i = 0; i < literal_num; i++) {
		if (strcmp(literal_table[i].symbol, label) == 0) {
			return -1;
		}
	}
	strcpy_s(literal_table[literal_num].symbol,sizeof(literal_table[literal_num].symbol), label);
	literal_table[literal_num].addr = -1;	// �ּҴ� ���� ����(-1)

	// literal ���� ���� ����
	literal_line_index[literal_num] = token_line - 1;  // ���� �� �ε��� (token_table ����)

	literal_num++;
	return 0;	
}

/* ----------------------------------------------------------------------------------
* ���� : label�� ���� �ּҸ� ��ȯ�ϴ� �Լ��̴�.
* �Ű� : �˻� ���� idx, ������ label
* ��ȯ : ���� : �ּ�, ���� :-1)
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
* ���� : LTORG ��ɾ ó���ϱ� ���� �Լ��̴�.
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
* ���� : nixbpe�� ���� 
* �Ű� : ��ū, ����
* -----------------------------------------------------------------------------------
*/
void set_nixbpe( token* t, int format) {
	t->nixbpe = 0;
	if (format == 2) return;
	if (format == 4) {
		t->nixbpe |= 0x01; // e = 1
	}

	// operand[0] ����
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
		// operand ������ �⺻���� ni = 11
		t->nixbpe |= (1 << 5) | (1 << 4);
	}

	// x ���
	if (t->operand[1] != NULL && strcmp(t->operand[1], "X") == 0) {
		t->nixbpe |= (1 << 3);  // x = 1
	}
	
	// p, b ������ format 3/4�� ���� �ش�
		int target = 0;
		int disp = 0;

	if (t->operand[0] != NULL && t->operand[0][0] != '#') {
		if (format == 3) {
			disp = target - (t->addr + 3);
			if (disp >= -2048 && disp < 2048) {
				t->nixbpe |= (1 << 1);  // p = 1
			}
			else {
				t->nixbpe |= (1 << 2);  // b = 1 (���� base ó�� �ʿ�)
			}
		}
	}
	
}

/* ----------------------------------------------------------------------------------
* ���� : ������ �Ҵ�� �ּҸ� ã�� �Լ��̴�. SECT�� ������ �ּҸ� ã�� ���� ���ȴ�.
* �Ű� : csect ���� �ε���
* ��ȯ : �ּ�
* -----------------------------------------------------------------------------------
*/
int get_last_allocated_addr(int idx) {
	int sect_start = 0;

	// idx�� ���° sect�� �ִ��� Ȯ��
	for (int i = 1; i < csect_num; i++) {
		if (csect_start_token_line[i] >= idx) break;
		sect_start = csect_start_token_line[i];
	}

	int max_addr = -1;

	// �Ϲ� ��ɾ� ���� ���� ������ �ּ�
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

		if (max_addr != -1) break;  // ���� ����� �� �ϳ��� ã���� �Ǵϱ�
	}

	// ���ͷ� ó�� (�ش� ���� �ȿ� �ִ� �͸�)
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


