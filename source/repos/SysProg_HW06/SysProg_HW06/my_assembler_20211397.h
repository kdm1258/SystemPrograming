/*
 * my_assembler 함수를 위한 변수 선언 및 매크로를 담고 있는 헤더 파일이다.
 *
 */
#define MAX_INST 256
#define MAX_LINES 5000

#define MAX_COLUMNS 4
#define MAX_OPERAND 3

#define MAX_LINE_LENGTH 100

 /*
 * instruction 목록을 저장하는 구조체이다.
 * instruction 목록 파일로부터 정보를 받아와서 생성한다.
 * instruction 목록 파일에는 라인별로 하나의 instruction을 저장한다.
 *
 */
typedef struct _inst
{
	char str[10];			// 명령어 이름
	unsigned char op;		// 명령어 코드
	int format;				// 명령어 형식
	int ops;				// 피연산자 갯수
} inst;


// 기계어를 관리하는 테이블
inst* inst_table[MAX_INST];
int inst_index;


// 어셈블리 할 소스코드를 파일로부터 불러와 라인별로 관리하는 테이블
char* input_data[MAX_LINES];
static int line_num;

int label_num;
int literal_num;


/*
 * 어셈블리 할 소스코드를 토큰으로 변환하여 저장하는 구조체 변수이다.
 * operator 변수명은 renaming을 허용한다.
 */
typedef struct _token {
	char* label;		
	char* oper;			//operator
	char* operand[MAX_OPERAND];	
	char comment[100];
	char nixbpe;    // 하위 6비트 사용 _ _ n i x b p e
	int addr;	// 주소
} token;

// 어셈블리 할 소스코드를 5000라인까지 관리하는 테이블
token* token_table[MAX_LINES];
static int token_line;

/*
 * 심볼을 관리하는 구조체이다.
 * 심볼 테이블은 심볼 이름, 심볼의 위치로 구성된다.
 */
typedef struct _symbol {
	char symbol[10];
	int addr;
} symbol;

/*
 * 리터럴을 관리하는 구조체이다.
 * 리터럴 테이블은 리터럴의 이름, 리터럴의 위치로 구성된다.
 */
typedef struct _literal {
	char* literal;
	int addr;
} literal;

symbol sym_table[MAX_LINES];
symbol literal_table[MAX_LINES];


/**
 * 오브젝트 코드 전체에 대한 정보를 담는 구조체이다.
 * Header Record, Define Recode,
 * Modification Record 등에 대한 정보를 모두 포함하고 있어야 한다. 이
 * 구조체 변수 하나만으로 object code를 충분히 작성할 수 있도록 구조체를 직접
 * 정의해야 한다.
 */
typedef struct _object_code {
	char record_type;	//'H', 'T', 'M', 'E'
	int locctr;		// 주소
	int length;		// 길이
	char object_code[100];	// 오브젝트 코드
} object_code;


object_code obj_codes[MAX_LINES];
static int obj_count = 0;	// 오브젝트 코드 개수

static int start_addr = 0;	// 프로그램 시작 주소
static int locctr;			// 현재 주소
static int prog_length = 0;	// 프로그램 길이
static int csect_start_symbol_num[MAX_LINES];	// CSECT 시작 심볼 번호
static int csect_num = 0;	// CSECT 개수
static int literal_line_index[100];	//csect별 리터럴 나누기용
static int csect_start_token_line[10]; // 섹터별 시작 토큰 인덱스


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

// 추가 함수
int insert_symtab(int start_symtable, const char* label, int addr);
int insert_literaltab(const char* label);
int get_symval(int start_symtable, const char* label);
void assign_littab_addresses();
void set_nixbpe(token* t, int format);
int get_last_allocated_addr(int idx);