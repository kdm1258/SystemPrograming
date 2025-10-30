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

	make_opcode_output(NULL);	// ���� �ڵ� ���

	// make_symtab_output("output_symtab.txt");         //  ���� ������ ��� ����
	// make_literaltab_output("output_littab.txt");     //  ���� ������ ��� ����

	if (assem_pass2() < 0)
	{
		printf(" assem_pass2: �н�2 �������� �����Ͽ����ϴ�.  \n");
		return -1;
	}

	// make_objectcode_output("output_objectcode.txt"); //  ���� ������ ��� ����
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
		if (format[1] == '/') {
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
		strcpy_s(tok->comment, sizeof(tok->comment), tmp_comment);
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
	/* input_data�� ���ڿ��� ���پ� �Է� �޾Ƽ�
	 * token_parsing()�� ȣ���Ͽ� _token�� ����
	 */
	for (int i = 0; i < line_num; i++) {
		if (token_parsing(input_data[i]) < 0) {
			printf("token_parsing: ��ū �Ľ̿� �����Ͽ����ϴ�.\n");
			return -1;
		}
	}
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

		// operand ���ڿ� ����� (���� ����)
		char operand_str[100] = "";
		for (int j = 0; j < MAX_OPERAND; j++) {
			if (t->operand[j] != NULL) {
				if (operand_str[0] != '\0') {
					strcat_s(operand_str, sizeof(operand_str), ",");
				}
				strcat_s(operand_str, sizeof(operand_str), t->operand[j]);
			}
		}

		// opcode ���ڿ� ����� (���� ����)
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
	/* add your code here */
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
	/* add your code here */
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

	/* add your code here */

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
	/* add your code here */
}

