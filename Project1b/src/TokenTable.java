import java.util.ArrayList;

/**
 * ����ڰ� �ۼ��� ���α׷� �ڵ带 �ܾ�� ���� �� ��, �ǹ̸� �м��ϰ�, ���� �ڵ�� ��ȯ�ϴ� ������ �Ѱ��ϴ� Ŭ�����̴�. <br>
 * pass2���� object code�� ��ȯ�ϴ� ������ ȥ�� �ذ��� �� ���� symbolTable�� instTable�� ������ �ʿ��ϹǷ� �̸� ��ũ��Ų��.<br>
 * section ���� �ν��Ͻ��� �ϳ��� �Ҵ�ȴ�.
 *
 */
public class TokenTable {
	public static final int MAX_OPERAND=3;
	
	/* bit ������ �������� ���� ���� */
	public static final int nFlag=32;
	public static final int iFlag=16;
	public static final int xFlag=8;
	public static final int bFlag=4;
	public static final int pFlag=2;
	public static final int eFlag=1;
	
	/* Token�� �ٷ� �� �ʿ��� ���̺���� ��ũ��Ų��. */
	SymbolTable symTab;
	InstTable instTab;
	LiteralTable litTab;
	

	/** �� line�� �ǹ̺��� �����ϰ� �м��ϴ� ����. */
	ArrayList<Token> tokenList;
	
	/**
	 * �ʱ�ȭ�ϸ鼭 symTable�� instTable�� ��ũ��Ų��.
	 * @param symTab : �ش� section�� ����Ǿ��ִ� symbol table
	 * @param instTab : instruction ���� ���ǵ� instTable
	 */
	public TokenTable(SymbolTable symTab, InstTable instTab, LiteralTable litTab) {
		this.tokenList = new ArrayList<>();
		this.symTab = symTab;
		this.instTab = instTab;
		this.litTab = litTab;
	}
	
	/**
	 * �Ϲ� ���ڿ��� �޾Ƽ� Token������ �и����� tokenList�� �߰��Ѵ�.
	 * @param line : �и����� ���� �Ϲ� ���ڿ�
	 */
	public void putToken(String line) {
		tokenList.add(new Token(line, instTab));
	}
	
	/**
	 * tokenList���� index�� �ش��ϴ� Token�� �����Ѵ�.
	 * @param index
	 * @return : index��ȣ�� �ش��ϴ� �ڵ带 �м��� Token Ŭ����
	 */
	public Token getToken(int index) {
		return tokenList.get(index);
	}
	
	/**
	 * Pass2 �������� ����Ѵ�.
	 * instruction table, symbol table ���� �����Ͽ� objectcode�� �����ϰ�, �̸� �����Ѵ�.
	 * @param index
	 */
	public void makeObjectCode(int index){
		Token token = tokenList.get(index);
		//�ּ� ó��
		if (token.operator == null || token.operator.isEmpty()) return;

		// ����: RESB, RESW ���� object code ����
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
			// ���� ���ͷ����� Ȯ��
			if (token.operand[0].matches("[0-9]+")) {
				value = Integer.parseInt(token.operand[0]);
			} else {
				// label (symbol)�� ��� symbol table���� �ּҰ� ��ȸ
				value = symTab.searchSymbol(token.operand[0]);
				if (value == -1) {
					value = 0;	//�ɺ��� ã�� ���Ѱ�� 0���� ä������
				}
			}

			token.objectCode = String.format("%06X", value);
			return;
		}
		// �⺻ ��ɾ�
		if(token.operator.startsWith("+")) {
			// 4���� ó��
			Instruction inst = instTab.searchOpcode(token.operator.substring(1));		//+����
			if (inst != null) {
				int opcode = inst.opcode;
				// 4������ n i e Flag == 1
				token.setFlag(nFlag,1);
				token.setFlag(iFlag,1);
				token.setFlag(eFlag,1);
				if(token.operand[1] != null && token.operand[1].equals("X")) token.setFlag(xFlag,1);

				// ���� 6bit opcode + nixbpe + ��������Ʈ 0
				int opAndFlags = (opcode<<24) | (token.nixbpe<<20);

				// object code�� 8�ڸ�: opcode/nixbpe(2byte) + address(3byte)
				token.objectCode = String.format("%08X", opAndFlags);
			}
		}
		else {
			int targetAddr = 0;
			Instruction inst = instTab.searchOpcode(token.operator);
			if(inst != null){
				if(token.operand[0] == null){
					//RSUBó��
					if(token.operator.equals("RSUB")){
						int opcode = inst.opcode;
						token.setFlag(nFlag,1);
						token.setFlag(iFlag,1);
						int opAndFlags = (opcode<<16) | (token.nixbpe<<12);
						token.objectCode = String.format("%06X", opAndFlags);
					}
				}
				// imediate ��ɾ�
				else if(token.operand[0].startsWith("#")){
					int opcode = inst.opcode;
					token.setFlag(nFlag,0);
					token.setFlag(iFlag,1);
					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | Integer.parseInt(token.operand[0].substring(1));

					token.objectCode = String.format("%06X", opAndFlags);
					return;
				}
				//literal ��ɾ�
				else if(token.operand[0].startsWith("=")){
					int opcode = inst.opcode;
					token.setFlag(pFlag,1);
					token.setFlag(nFlag,1);
					token.setFlag(iFlag,1);
					targetAddr = litTab.getLocation(token.operand[0]);
					if(targetAddr == -1 ){
						throw new RuntimeException("literal�� ã�� ���߽��ϴ�\n");
					}
					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | (targetAddr - (token.location+3));

					token.objectCode = String.format("%06X", opAndFlags);
				}
				// indirect ��ɾ�
				else if(token.operand[0].startsWith("@")){
					int opcode = inst.opcode;
					token.setFlag(pFlag,1);
					token.setFlag(nFlag,1);
					targetAddr = symTab.searchSymbol(token.operand[0].substring(1));

					int opAndFlags = (opcode<<16) | (token.nixbpe<<12) | ((targetAddr - (token.location+3)) & 0x0FFF);
					if(targetAddr == -1 ){
						throw new RuntimeException("symbol�� ã�� ���߽��ϴ�\n");
					}
					token.objectCode = String.format("%06X", opAndFlags);
				}
				//�Ϲ� ��ɾ�
				else{
					int opcode = inst.opcode;
					//format 3
					if(inst.format == 3) {
						token.setFlag(pFlag, 1);
						token.setFlag(nFlag, 1);
						token.setFlag(iFlag, 1);
						targetAddr = symTab.searchSymbol(token.operand[0]);
						if (targetAddr == -1) {
							throw new RuntimeException(String.format("%s symbol�� ã�� ���߽��ϴ�\n", token.operand[0]));
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
	 * index��ȣ�� �ش��ϴ� object code�� �����Ѵ�.
	 * @param index
	 * @return : object code
	 */
	public String getObjectCode(int index) {
		return tokenList.get(index).objectCode;
	}
	
}

/**
 * �� ���κ��� ����� �ڵ带 �ܾ� ������ ������ ��  �ǹ̸� �ؼ��ϴ� ���� ���Ǵ� ������ ������ �����Ѵ�. 
 * �ǹ� �ؼ��� ������ pass2���� object code�� �����Ǿ��� ���� ����Ʈ �ڵ� ���� �����Ѵ�.
 */
class Token{
	//�ǹ� �м� �ܰ迡�� ���Ǵ� ������
	int location;
	String label;
	String operator;
	String[] operand;
	String comment;
	char nixbpe;

	InstTable instTable;

	// object code ���� �ܰ迡�� ���Ǵ� ������ 
	String objectCode;
	int byteSize;
	
	/**
	 * Ŭ������ �ʱ�ȭ �ϸ鼭 �ٷ� line�� �ǹ� �м��� �����Ѵ�. 
	 * @param line ��������� ����� ���α׷� �ڵ�
	 */
	public Token(String line, InstTable instTable) {
		this.operand = new String[TokenTable.MAX_OPERAND];
		this.comment = "";
		this.instTable = instTable;
		parsing(line);
	}
	
	/**
	 * line�� �������� �м��� �����ϴ� �Լ�. Token�� �� ������ �м��� ����� �����Ѵ�.
	 * @param line ��������� ����� ���α׷� �ڵ�.
	 */
	public void parsing(String line) {
		// �ּ� ó��
		if (line.startsWith(".")) {
			this.comment = line;
			return;
		}

		// �⺻ �ʱ�ȭ
		label = null;
		operator = null;
		comment = "";
		for (int i = 0; i < TokenTable.MAX_OPERAND; i++) operand[i] = null;

		String trimmedLine = line.trim();
		String[] parts;
		String operandPart = null;
		int numOperands = TokenTable.MAX_OPERAND;

		// label�� ���� ���: '\t'�� ����
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

		// �ǿ����� ���� �ľ�
		if (operator != null) {
			String checkOp = operator.startsWith("+") ? operator.substring(1) : operator;
			Instruction inst = instTable.searchOpcode(checkOp);
			if (inst != null) {
				numOperands = inst.operandCnt;
			}
		}

		// operand ������ 0�̸� �ڴ� �ּ� ó��
		String operandsAndComment = operandPart != null ? operandPart : "";
		String[] tokens = operandsAndComment.split("[ \t]+", 2);
		String rawOperands = (tokens.length > 0) ? tokens[0] : "";
		String trailingComment = (tokens.length > 1) ? tokens[1] : "";

		// �ǿ����� ����
		if (numOperands > 0 && rawOperands.length() > 0) {
			String[] ops = rawOperands.split(",");
			for (int i = 0; i < ops.length && i < TokenTable.MAX_OPERAND; i++) {
				operand[i] = ops[i].trim();
			}
		}

		// �ּ� ����
		if (!trailingComment.isEmpty()) {
			comment = trailingComment.strip();
		}
	}


	/** 
	 * n,i,x,b,p,e flag�� �����Ѵ�. <br><br>
	 * 
	 * ��� �� : setFlag(nFlag, 1); <br>
	 *   �Ǵ�     setFlag(TokenTable.nFlag, 1);
	 * 
	 * @param flag : ���ϴ� ��Ʈ ��ġ
	 * @param value : ����ְ��� �ϴ� ��. 1�Ǵ� 0���� �����Ѵ�.
	 */
	public void setFlag(int flag, int value) {
		if (value == 1) {
			nixbpe |= flag;    // �ش� ��Ʈ�� �� (OR ����)
		} else {
			nixbpe &= ~flag;   // �ش� ��Ʈ�� �� (AND with NOT)
		}
	}
	
	/**
	 * ���ϴ� flag���� ���� ���� �� �ִ�. flag�� ������ ���� ���ÿ� �������� �÷��׸� ��� �� ���� �����ϴ� <br><br>
	 * 
	 * ��� �� : getFlag(nFlag) <br>
	 *   �Ǵ�     getFlag(nFlag|iFlag)
	 * 
	 * @param flags : ���� Ȯ���ϰ��� �ϴ� ��Ʈ ��ġ
	 * @return : ��Ʈ��ġ�� �� �ִ� ��. �÷��׺��� ���� 32, 16, 8, 4, 2, 1�� ���� ������ ����.
	 */
	public int getFlag(int flags) {
		return nixbpe & flags;
	}
}
