import java.util.ArrayList;

/**
 * 사용자가 작성한 프로그램 코드를 단어별로 분할 한 후, 의미를 분석하고, 최종 코드로 변환하는 과정을 총괄하는 클래스이다. <br>
 * pass2에서 object code로 변환하는 과정은 혼자 해결할 수 없고 symbolTable과 instTable의 정보가 필요하므로 이를 링크시킨다.<br>
 * section 마다 인스턴스가 하나씩 할당된다.
 *
 */
public class TokenTable {
	public static final int MAX_OPERAND=3;
	
	/* bit 조작의 가독성을 위한 선언 */
	public static final int nFlag=32;
	public static final int iFlag=16;
	public static final int xFlag=8;
	public static final int bFlag=4;
	public static final int pFlag=2;
	public static final int eFlag=1;
	
	/* Token을 다룰 때 필요한 테이블들을 링크시킨다. */
	SymbolTable symTab;
	InstTable instTab;
	LiteralTable litTab;
	

	/** 각 line을 의미별로 분할하고 분석하는 공간. */
	ArrayList<Token> tokenList;
	
	/**
	 * 초기화하면서 symTable과 instTable을 링크시킨다.
	 * @param symTab : 해당 section과 연결되어있는 symbol table
	 * @param instTab : instruction 명세가 정의된 instTable
	 */
	public TokenTable(SymbolTable symTab, InstTable instTab, LiteralTable litTab) {
		this.tokenList = new ArrayList<>();
		this.symTab = symTab;
		this.instTab = instTab;
		this.litTab = litTab;
	}
	
	/**
	 * 일반 문자열을 받아서 Token단위로 분리시켜 tokenList에 추가한다.
	 * @param line : 분리되지 않은 일반 문자열
	 */
	public void putToken(String line) {
		tokenList.add(new Token(line, instTab));
	}
	
	/**
	 * tokenList에서 index에 해당하는 Token을 리턴한다.
	 * @param index
	 * @return : index번호에 해당하는 코드를 분석한 Token 클래스
	 */
	public Token getToken(int index) {
		return tokenList.get(index);
	}
	
	/**
	 * Pass2 과정에서 사용한다.
	 * instruction table, symbol table 등을 참조하여 objectcode를 생성하고, 이를 저장한다.
	 * @param index
	 */
	public void makeObjectCode(int index){
		Token token = tokenList.get(index);
		//주석 처리
		if (token.operator == null || token.operator.isEmpty()) return;

		// 예외: RESB, RESW 등은 object code 없음
		if (token.operator.equals("RESB") || token.operator.equals("RESW") ||
				token.operator.equals("EXTDEF") || token.operator.equals("EXTREF") ||
				token.operator.equals("CSECT") || token.operator.equals("START") ||
				token.operator.equals("EQU") || token.operator.equals("LTORG")) {
			token.objectCode = "";
			return;
		}
		//BYTE
		if(token.operator.equals("BYTE")){
			if(token.operand[0].startsWith("C'") && token.operand[0].endsWith("'")){
				String str = token.operand[0].substring(2, token.operand[0].length() - 1);
				StringBuilder hex = new StringBuilder();
				for(char c : str.toCharArray()){
					hex.append(String.format("%02X",(int)c));
				}
				token.objectCode = hex.toString();
			}
			else if(token.operand[0].startsWith("X'") && token.operand[0].endsWith("'")){
				token.objectCode = token.operand[0].substring(2, token.operand[0].length()-1);
			}
			return;
		}
		// WORD
		if(token.operator.equals("WORD")){
			int value;
			// 숫자 리터럴인지 확인
			if (token.operand[0].matches("[0-9]+")) {
				value = Integer.parseInt(token.operand[0]);
			} else {
				// label (symbol)일 경우 symbol table에서 주소값 조회
				value = symTab.searchSymbol(token.operand[0]);
				if (value == -1) {
					value = 0;	//심볼을 찾지 못한경우 0으로 채워넣음
				}
			}

			token.objectCode = String.format("%06X", value);
			return;
		}
		// 기본 명령어
		if(token.operator.startsWith("+")) {
			// 4형식 처리
			Instruction inst = instTab.searchOpcode(token.operator.substring(1));		//+제거
			if (inst != null) {
				int opcode = inst.opcode;
				// 4형식은 n i e Flag == 1
				token.setFlag(nFlag,1);
				token.setFlag(iFlag,1);
				token.setFlag(eFlag,1);
				if(token.operand[1] != null && token.operand[1].equals("X")) token.setFlag(xFlag,1);

				// 상위 6bit opcode + nixbpe + 나머지비트 0
				int opAndFlags = (opcode<<24) | (token.nixbpe<<20);

				// object code는 8자리: opcode/nixbpe(2byte) + address(3byte)
				token.objectCode = String.format("%08X", opAndFlags);
			}
		}
		else {
			int targetAddr = 0;
			Instruction inst = instTab.searchOpcode(token.operator);
			if(inst != null){
				if(token.operand[0] == null){
					//RSUB처리
					if(token.operator.equals("RSUB")){
						int opcode = inst.opcode;
						token.setFlag(nFlag,1);
						token.setFlag(iFlag,1);
						int opAndFlags = (opcode<<16) | (token.nixbpe<<12);
						token.objectCode = String.format("%06X", opAndFlags);
					}
				}
				// imediate 명령어
				else if(token.operand[0].startsWith("#")){
					int opcode = inst.opcode;
					token.setFlag(nFlag,0);
					token.setFlag(iFlag,1);
					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | Integer.parseInt(token.operand[0].substring(1));

					token.objectCode = String.format("%06X", opAndFlags);
					return;
				}
				//literal 명령어
				else if(token.operand[0].startsWith("=")){
					int opcode = inst.opcode;
					token.setFlag(pFlag,1);
					token.setFlag(nFlag,1);
					token.setFlag(iFlag,1);
					targetAddr = litTab.getLocation(token.operand[0]);
					if(targetAddr == -1 ){
						throw new RuntimeException("literal을 찾지 못했습니다\n");
					}
					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | (targetAddr - (token.location+3));

					token.objectCode = String.format("%06X", opAndFlags);
				}
				// indirect 명령어
				else if(token.operand[0].startsWith("@")){
					int opcode = inst.opcode;
					token.setFlag(pFlag,1);
					token.setFlag(nFlag,1);
					targetAddr = symTab.searchSymbol(token.operand[0].substring(1));

					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | ((targetAddr - (token.location+3)) & 0x0FFF);
					if(targetAddr == -1 ){
						throw new RuntimeException("symbol을 찾지 못했습니다\n");
					}
					token.objectCode = String.format("%06X", opAndFlags);
				}
				//일반 명령어
				else{
					int opcode = inst.opcode;
					//format 3
					if(inst.format == 3) {
						token.setFlag(pFlag, 1);
						token.setFlag(nFlag, 1);
						token.setFlag(iFlag, 1);
						targetAddr = symTab.searchSymbol(token.operand[0]);
						if (targetAddr == -1) {
							throw new RuntimeException(String.format("%s symbol을 찾지 못했습니다\n", token.operand[0]));
						}
						int opAndFlags = (opcode << 16) | (token.nixbpe << 12) |  ((targetAddr - (token.location+3)) & 0x0FFF);
						token.objectCode = String.format("%06X", opAndFlags);
					}
					//format2
					else if(inst.format == 2){
						int tmp = (opcode<<8);
						if (token.operand[0] != null) {
							switch (token.operand[0]) {
								case "A": tmp |= 0x00; break;
								case "X": tmp |= 0x10; break;
								case "L": tmp |= 0x20; break;
								case "B": tmp |= 0x30; break;
								case "S": tmp |= 0x40; break;
								case "T": tmp |= 0x50; break;
							}
						}
						if (token.operand[1] != null) {
							switch (token.operand[1]) {
								case "A": tmp |= 0x00; break;
								case "X": tmp |= 0x01; break;
								case "L": tmp |= 0x02; break;
								case "B": tmp |= 0x03; break;
								case "S": tmp |= 0x04; break;
								case "T": tmp |= 0x05; break;
							}
						}
						token.objectCode = String.format("%04X",tmp);
					}
				}
			}
		}
		return;
	}
	
	/** 
	 * index번호에 해당하는 object code를 리턴한다.
	 * @param index
	 * @return : object code
	 */
	public String getObjectCode(int index) {
		return tokenList.get(index).objectCode;
	}
	
}

/**
 * 각 라인별로 저장된 코드를 단어 단위로 분할한 후  의미를 해석하는 데에 사용되는 변수와 연산을 정의한다. 
 * 의미 해석이 끝나면 pass2에서 object code로 변형되었을 때의 바이트 코드 역시 저장한다.
 */
class Token{
	//의미 분석 단계에서 사용되는 변수들
	int location;
	String label;
	String operator;
	String[] operand;
	String comment;
	char nixbpe;

	InstTable instTable;

	// object code 생성 단계에서 사용되는 변수들 
	String objectCode;
	int byteSize;
	
	/**
	 * 클래스를 초기화 하면서 바로 line의 의미 분석을 수행한다. 
	 * @param line 문장단위로 저장된 프로그램 코드
	 */
	public Token(String line, InstTable instTable) {
		this.operand = new String[TokenTable.MAX_OPERAND];
		this.comment = "";
		this.instTable = instTable;
		parsing(line);
	}
	
	/**
	 * line의 실질적인 분석을 수행하는 함수. Token의 각 변수에 분석한 결과를 저장한다.
	 * @param line 문장단위로 저장된 프로그램 코드.
	 */
	public void parsing(String line) {
		// 주석 처리
		if (line.startsWith(".")) {
			this.comment = line;
			return;
		}

		// 기본 초기화
		label = null;
		operator = null;
		comment = "";
		for (int i = 0; i < TokenTable.MAX_OPERAND; i++) operand[i] = null;

		String trimmedLine = line.trim();
		String[] parts;
		String operandPart = null;
		int numOperands = TokenTable.MAX_OPERAND;

		// label이 없는 경우: '\t'로 시작
		if (line.startsWith("\t")) {
			trimmedLine = trimmedLine.strip();
			parts = trimmedLine.split("[ \t\n]+", 2);
			operator = (parts.length > 0) ? parts[0] : null;
			if (parts.length > 1) operandPart = parts[1];
		} else {
			parts = trimmedLine.split("[ \t\n]+", 3);
			label = (parts.length > 0) ? parts[0] : null;
			operator = (parts.length > 1) ? parts[1] : null;
			if (parts.length > 2) operandPart = parts[2];
		}

		// 피연산자 개수 파악
		if (operator != null) {
			String checkOp = operator.startsWith("+") ? operator.substring(1) : operator;
			Instruction inst = instTable.searchOpcode(checkOp);
			if (inst != null) {
				numOperands = inst.operandCnt;
			}
		}

		// operand 개수가 0이면 뒤는 주석 처리
		String operandsAndComment = operandPart != null ? operandPart : "";
		String[] tokens = operandsAndComment.split("[ \t]+", 2);
		String rawOperands = (tokens.length > 0) ? tokens[0] : "";
		String trailingComment = (tokens.length > 1) ? tokens[1] : "";

		// 피연산자 분해
		if (numOperands > 0 && rawOperands.length() > 0) {
			String[] ops = rawOperands.split(",");
			for (int i = 0; i < ops.length && i < TokenTable.MAX_OPERAND; i++) {
				operand[i] = ops[i].trim();
			}
		}

		// 주석 저장
		if (!trailingComment.isEmpty()) {
			comment = trailingComment.strip();
		}
	}


	/** 
	 * n,i,x,b,p,e flag를 설정한다. <br><br>
	 * 
	 * 사용 예 : setFlag(nFlag, 1); <br>
	 *   또는     setFlag(TokenTable.nFlag, 1);
	 * 
	 * @param flag : 원하는 비트 위치
	 * @param value : 집어넣고자 하는 값. 1또는 0으로 선언한다.
	 */
	public void setFlag(int flag, int value) {
		if (value == 1) {
			nixbpe |= flag;    // 해당 비트를 켬 (OR 연산)
		} else {
			nixbpe &= ~flag;   // 해당 비트를 끔 (AND with NOT)
		}
	}
	
	/**
	 * 원하는 flag들의 값을 얻어올 수 있다. flag의 조합을 통해 동시에 여러개의 플래그를 얻는 것 역시 가능하다 <br><br>
	 * 
	 * 사용 예 : getFlag(nFlag) <br>
	 *   또는     getFlag(nFlag|iFlag)
	 * 
	 * @param flags : 값을 확인하고자 하는 비트 위치
	 * @return : 비트위치에 들어가 있는 값. 플래그별로 각각 32, 16, 8, 4, 2, 1의 값을 리턴할 것임.
	 */
	public int getFlag(int flags) {
		return nixbpe & flags;
	}
}
