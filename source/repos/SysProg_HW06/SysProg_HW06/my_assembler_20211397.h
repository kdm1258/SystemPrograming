/*
 * my_assembler �Լ��� ���� ���� ���� �� ��ũ�θ� ��� �ִ� ��� �����̴�.
 *
 */
#define MAX_INST 256
#define MAX_LINES 5000

#define MAX_COLUMNS 4
#define MAX_OPERAND 3

#define MAX_LINE_LENGTH 100

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
int literal_num;


/*
 * ����� �� �ҽ��ڵ带 ��ū���� ��ȯ�Ͽ� �����ϴ� ����ü �����̴�.
 * operator �������� renaming�� ����Ѵ�.
 */
typedef struct _token {
	char* label;		
	char* oper;			//operator
	char* operand[MAX_OPERAND];	
	char comment[100];
	char nixbpe;    // ���� 6��Ʈ ��� _ _ n i x b p e
	int addr;	// �ּ�
} token;

// ����� �� �ҽ��ڵ带 5000���α��� �����ϴ� ���̺�
token* token_table[MAX_LINES];
static int token_line;

/*
 * �ɺ��� �����ϴ� ����ü�̴�.
 * �ɺ� ���̺��� �ɺ� �̸�, �ɺ��� ��ġ�� �����ȴ�.
 */
typedef struct _symbol {
	char symbol[10];
	int addr;
} symbol;

/*
 * ���ͷ��� �����ϴ� ����ü�̴�.
 * ���ͷ� ���̺��� ���ͷ��� �̸�, ���ͷ��� ��ġ�� �����ȴ�.
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
 */
typedef struct _object_code {
	char record_type;	//'H', 'T', 'M', 'E'
	int locctr;		// �ּ�
	int length;		// ����
	char object_code[100];	// ������Ʈ �ڵ�
} object_code;


object_code obj_codes[MAX_LINES];
static int obj_count = 0;	// ������Ʈ �ڵ� ����

static int start_addr = 0;	// ���α׷� ���� �ּ�
static int locctr;			// ���� �ּ�
static int prog_length = 0;	// ���α׷� ����
static int csect_start_symbol_num[MAX_LINES];	// CSECT ���� �ɺ� ��ȣ
static int csect_num = 0;	// CSECT ����
static int literal_line_index[100];	//csect�� ���ͷ� �������
static int csect_start_token_line[10]; // ���ͺ� ���� ��ū �ε���


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

// �߰� �Լ�
int insert_symtab(int start_symtable, const char* label, int addr);
int insert_literaltab(const char* label);
int get_symval(int start_symtable, const char* label);
void assign_littab_addresses();
void set_nixbpe(token* t, int format);
int get_last_allocated_addr(int idx);