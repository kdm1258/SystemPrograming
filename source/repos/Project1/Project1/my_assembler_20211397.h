/*
 * my_assembler �Լ��� ���� ���� ���� �� ��ũ�θ� ��� �ִ� ��� �����̴�.
 *
 */
#define MAX_INST 256
#define MAX_LINES 5000

#define MAX_COLUMNS 4
#define MAX_OPERAND 3

#define MAX_LINE_LENGTH 1000

 /*
 * instruction ����� �����ϴ� ����ü�̴�.
 * instruction ��� ���Ϸκ��� ������ �޾ƿͼ� �����Ѵ�.
 * instruction ��� ���Ͽ��� ���κ��� �ϳ��� instruction�� �����Ѵ�.
 *
 */
typedef struct _inst
{
	char str[10];			// ��ɾ� �̸�
	unsigned char op;		// ��ɾ� �ڵ�
	int format;				// ��ɾ� ����
	int ops;				// �ǿ����� ����
} inst;


// ��� �����ϴ� ���̺�
inst* inst_table[MAX_INST];
int inst_index;


// ����� �� �ҽ��ڵ带 ���Ϸκ��� �ҷ��� ���κ��� �����ϴ� ���̺�
char* input_data[MAX_LINES];
static int line_num;

int label_num;


/*
 * ����� �� �ҽ��ڵ带 ��ū���� ��ȯ�Ͽ� �����ϴ� ����ü �����̴�.
 * operator �������� renaming�� ����Ѵ�.
 */
typedef struct _token {
	char* label;
	char* oper;			//operator
	char* operand[MAX_OPERAND];
	char comment[100];
	//char nixbpe;    // ���� 6��Ʈ ��� _ _ n i x b p e
} token;

// ����� �� �ҽ��ڵ带 5000���α��� �����ϴ� ���̺�
token* token_table[MAX_LINES];
static int token_line;

/*
 * �ɺ��� �����ϴ� ����ü�̴�.
 * �ɺ� ���̺��� �ɺ� �̸�, �ɺ��� ��ġ�� �����ȴ�.
 * ���� ������Ʈ���� ��� ����
 */
typedef struct _symbol {
	char symbol[10];
	int addr;
} symbol;

/*
* ���ͷ��� �����ϴ� ����ü�̴�.
* ���ͷ� ���̺��� ���ͷ��� �̸�, ���ͷ��� ��ġ�� �����ȴ�.
* ���� ������Ʈ���� ��� ����
*/
typedef struct _literal {
	char* literal;
	int addr;
} literal;

symbol sym_table[MAX_LINES];
symbol literal_table[MAX_LINES];


/**
 * ������Ʈ �ڵ� ��ü�� ���� ������ ��� ����ü�̴�.
 * Header Record, Define Recode,
 * Modification Record � ���� ������ ��� �����ϰ� �־�� �Ѵ�. ��
 * ����ü ���� �ϳ������� object code�� ����� �ۼ��� �� �ֵ��� ����ü�� ����
 * �����ؾ� �Ѵ�.
 *
 * ���� ������Ʈ���� ��� ����
 */
typedef struct _object_code {
	/* add fields */
	char* obj_code; //�������� �ӽú���
} object_code;


static int locctr;
//--------------

static char* input_file;
static char* output_file;

int init_my_assembler(void);
int init_inst_file(char* inst_file);
int init_input_file(char* input_file);
int token_parsing(char* str);
int search_opcode(char* str);
static int assem_pass1(void);
void make_opcode_output(char* file_name);
void make_symtab_output(char* file_name);
void make_literaltab_output(char* filename);
static int assem_pass2(void);
void make_objectcode_output(char* file_name);