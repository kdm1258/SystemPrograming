import javax.naming.ldap.ExtendedRequest;
import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;


/**
 * Assembler : 
 * �� ���α׷��� SIC/XE �ӽ��� ���� Assembler ���α׷��� ���� ��ƾ�̴�.
 * ���α׷��� ���� �۾��� ������ ����. <br>
 * 1) ó�� �����ϸ� Instruction ���� �о�鿩�� assembler�� �����Ѵ�. <br>
 * 2) ����ڰ� �ۼ��� input ������ �о���� �� �����Ѵ�. <br>
 * 3) input ������ ������� �ܾ�� �����ϰ� �ǹ̸� �ľ��ؼ� �����Ѵ�. (pass1) <br>
 * 4) �м��� ������ �������� ��ǻ�Ͱ� ����� �� �ִ� object code�� �����Ѵ�. (pass2) <br>
 * 
 * <br><br>
 * �ۼ����� ���ǻ��� : <br>
 *  1) ���ο� Ŭ����, ���ο� ����, ���ο� �Լ� ������ �󸶵��� ����. ��, ������ ������ �Լ����� �����ϰų� ������ ��ü�ϴ� ���� �ȵȴ�.<br>
 *  2) ���������� �ۼ��� �ڵ带 �������� ������ �ʿ信 ���� ����ó��, �������̽� �Ǵ� ��� ��� ���� ����.<br>
 *  3) ��� void Ÿ���� ���ϰ��� ������ �ʿ信 ���� �ٸ� ���� Ÿ������ ���� ����.<br>
 *  4) ����, �Ǵ� �ܼ�â�� �ѱ��� ��½�Ű�� �� ��. (ä������ ����. �ּ��� ���Ե� �ѱ��� ��� ����)<br>
 * 
 * <br><br>
 *  + �����ϴ� ���α׷� ������ ��������� �����ϰ� ���� �е��� ������ ��� �޺κп� ÷�� �ٶ��ϴ�. ���뿡 ���� �������� ���� �� �ֽ��ϴ�.
 */
public class Assembler {
	/** instruction ���� ������ ���� */
	InstTable instTable;
	/** �о���� input ������ ������ �� �� �� �����ϴ� ����. */
	ArrayList<String> lineList;
	/** ���α׷��� section���� symbol table�� �����ϴ� ����*/
	ArrayList<SymbolTable> symtabList;
	/** ���α׷��� section���� ���α׷��� �����ϴ� ����*/
	ArrayList<TokenTable> tokenList;
	/** ���α׷��� section���� ���ͷ��� �����ϴ� ����*/
	ArrayList<LiteralTable> litTabList;
	/**
	 * Token, �Ǵ� ���þ ���� ������� ������Ʈ �ڵ���� ��� ���·� �����ϴ� ����. <br>
	 * �ʿ��� ��� String ��� ������ Ŭ������ �����Ͽ� ArrayList�� ��ü�ص� ������.
	 */
	ArrayList<String> codeList;

	/**exter��ҵ��� �����ϴ� ����Ʈ*/
	ArrayList<Map<String, Integer>> extDefList;
	ArrayList<ArrayList<String>> extRefList;

	/**
	 * Ŭ���� �ʱ�ȭ. instruction Table�� �ʱ�ȭ�� ���ÿ� �����Ѵ�.
	 * 
	 * @param instFile : instruction ���� �ۼ��� ���� �̸�. 
	 */
	public Assembler(String instFile) {
		instTable = new InstTable(instFile);
		lineList = new ArrayList<String>();
		codeList = new ArrayList<String>();
		extDefList = new ArrayList<Map<String, Integer>>();
		extRefList = new ArrayList<ArrayList<String>>();
		symtabList = new ArrayList<SymbolTable>();
		tokenList = new ArrayList<TokenTable>();
		litTabList = new ArrayList<LiteralTable>();
	}

	/** 
	 * ������� ���� ��ƾ
	 */
	public static void main(String[] args) {
		Assembler assembler = new Assembler("inst_table.txt");
		assembler.loadInputFile("input.txt");
		
		assembler.pass1();
		assembler.printSymbolTable("output_symtab.txt");
		assembler.printLiteralTable("output_littab.txt");
		
		assembler.pass2();
		assembler.printObjectCode("output_objectcode.txt");
		
	}


	/**
	 * inputFile�� �о�鿩�� lineList�� �����Ѵ�.<br>
	 * @param inputFile : input ���� �̸�.
	 */
	private void loadInputFile(String inputFile) {
		try(BufferedReader reader = new BufferedReader(new FileReader(inputFile))){
			String line;
			while((line = reader.readLine()) != null){
				if(!line.isEmpty()){
					lineList.add(line);
				}
			}
		}
		catch (IOException e) {
            e.printStackTrace();
        }

    }

	/**
	 * �ۼ��� SymbolTable���� ������¿� �°� ����Ѵ�.
	 * @param fileName : ����Ǵ� ���� �̸�
	 */
	private void printSymbolTable(String fileName) {
		try (PrintWriter writer = new PrintWriter(fileName)) {
			for (int section = 0; section < symtabList.size(); section++) {
				SymbolTable symTab = symtabList.get(section);

				for (int i = 0; i < symTab.symbolList.size(); i++) {
					String symbol = symTab.symbolList.get(i);
					int address = symTab.locationList.get(i);
					writer.printf("%-10s\t%X\n", symbol, address);  // 16���� 4�ڸ� ���
				}

				writer.println();
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * �ۼ��� LiteralTable���� ������¿� �°� ����Ѵ�.<br>
	 * @param fileName : ����Ǵ� ���� �̸�
	 */
	private void printLiteralTable(String fileName) {
		try (PrintWriter writer = new PrintWriter(fileName)) {
			for (int j = 0; j < litTabList.size(); j++) {
				LiteralTable litTab = litTabList.get(j);
				for (int i = 0; i < litTab.literalList.size(); i++) {
					String literal = litTab.literalList.get(i);
					int address = litTab.locationList.get(i);

					// =C'EOF' �� EOF �� ���
					String cleanLiteral = literal;
					if (literal.startsWith("=C'") && literal.endsWith("'")) {
						cleanLiteral = literal.substring(3, literal.length() - 1);
					} else if (literal.startsWith("=X'") && literal.endsWith("'")) {
						cleanLiteral = literal.substring(3, literal.length() - 1);
					} else if (literal.matches("=\\d+")) {
						cleanLiteral = literal.substring(1); // =5 �� 5
					}

					writer.printf("%-10s\t%X\n", cleanLiteral, address);
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}


	/** 
	 * pass1 ������ �����Ѵ�.<br>
	 *   1) ���α׷� �ҽ��� ��ĵ�Ͽ� ��ū������ �и��� �� ��ū���̺� ����<br>
	 *   2) label�� symbolTable�� ����<br>
	 *   <br><br>
	 *    ���ǻ��� : SymbolTable�� TokenTable�� ���α׷��� section���� �ϳ��� ����Ǿ�� �Ѵ�.
	 */
	private void pass1() {
		SymbolTable symTab = new SymbolTable();
		LiteralTable litTab = new LiteralTable();
		TokenTable tokTab = new TokenTable(symTab, instTable, litTab);

		int locctr = 0;
		boolean startEncountered = false;

		for (String line : lineList) {
			tokTab.putToken(line);
			Token token = tokTab.getToken(tokTab.tokenList.size() - 1);

			if (line.startsWith(".")) continue; // �ּ� ���� ����

			// START ó��
			if (!startEncountered && "START".equals(token.operator)) {
				if (token.operand[0] != null)
					locctr = Integer.parseInt(token.operand[0], 16);
				token.location = locctr;
				startEncountered = true;
				if (token.label != null)
					symTab.putSymbol(token.label, locctr);
				continue;
			}

			// CSECT ó��
			if ("CSECT".equals(token.operator)) {
				locctr = litTab.assignAddresses(locctr);
				token.location = locctr;

				//���� table�� ���� ���Ϳ� ����
				symtabList.add(symTab);
				tokenList.add(tokTab);
				litTabList.add(litTab);

				//���ο� ���� table ����
				symTab = new SymbolTable();
				litTab = new LiteralTable();
				tokTab = new TokenTable(symTab, instTable, litTab);
				locctr = 0;

				symTab.putSymbol(token.label, locctr);

				continue;
			}

			// �ܺ� ����/�����̹Ƿ� locctr ���� ����
			if ("EXTREF".equals(token.operator)) {
				ArrayList<String> curExtRefList = new ArrayList<>();
				for(int i = 0 ; i < TokenTable.MAX_OPERAND ; i++){
					if(token.operand[i] != null) {
						curExtRefList.add(token.operand[i]);
					}
				}
				extRefList.add(curExtRefList);
				continue;
			}
			if ("EXTDEF".equals(token.operator)) {
				Map<String, Integer> curExtDefMap = new HashMap<>();
				for (int i = 0; i < TokenTable.MAX_OPERAND; i++) {
					if (token.operand[i] != null) {
						curExtDefMap.put(token.operand[i], -1); // ���߿� �ּ� ä��
					}
				}
				extDefList.add(curExtDefMap);
				continue;
			}

			// EQU ó��
			if ("EQU".equals(token.operator)) {
				int value = 0;
				// �ǿ����ڰ� *�ΰ�� ���� locctr�Ҵ�
				if ("*".equals(token.operand[0])) {
					value = locctr;
				}
				//�ǿ����ڰ� ������ ���
				else if (token.operand[0].matches("[0-9]+")) {
					value = Integer.parseInt(token.operand[0]);
				}
				//������ ���Ե� ��� ("-"����)
				else if (token.operand[0].contains("-")) {
					String[] parts = token.operand[0].split("-");
					int left = symTab.searchSymbol(parts[0]);
					int right = symTab.searchSymbol(parts[1]);
					value = left - right;
				}
				//�ǿ����ڰ� �ɺ�
				else {
					value = symTab.searchSymbol(token.operand[0]) == -1 ? 0 : symTab.searchSymbol(token.operand[0]);
				}
				if (token.label != null)
					symTab.putSymbol(token.label, value);
				token.location = locctr;
				continue;
			}

			// LTORG ó��
			if ("LTORG".equals(token.operator)) {
				locctr = litTab.assignAddresses(locctr);
				continue;
			}

			// ���ͷ� �߰�
			if (token.operand[0] != null && token.operand[0].startsWith("=")) {
				litTab.putLiteral(token.operand[0]);
			}

			// label ����
			if (token.label != null)
				symTab.putSymbol(token.label, locctr);

			// �ּ� ����
			token.location = locctr;

			// byteSize ���
			int byteSize = 3;
			if ("BYTE".equals(token.operator)) {
				String v = token.operand[0];
				if (v.startsWith("C'")) {
					byteSize = v.length() - 3;
				} else if (v.startsWith("X'")) {
					byteSize = (v.length() - 3) / 2;
				}
			} else if ("WORD".equals(token.operator)) {
				byteSize = 3;
			} else if ("RESB".equals(token.operator)) {
				byteSize = Integer.parseInt(token.operand[0]);
			} else if ("RESW".equals(token.operator)) {
				byteSize = Integer.parseInt(token.operand[0]) * 3;
			} else {
				String checkOp = token.operator.startsWith("+") ? token.operator.substring(1) : token.operator;
				Instruction inst = instTable.searchOpcode(checkOp);
				if (inst != null)
					byteSize = token.operator.startsWith("+") ? 4 : inst.format;
				else byteSize = 0;
			}
			token.byteSize = byteSize;
			locctr += byteSize;
		}

		// ������ LTORG ó��
		locctr = litTab.assignAddresses(locctr);

		// ���̺� ����
		symtabList.add(symTab);
		tokenList.add(tokTab);
		litTabList.add(litTab);
	}



	/**
	 * pass2 ������ �����Ѵ�.<br>
	 *   1) �м��� ������ �������� object code�� �����Ͽ� codeList�� ����.
	 */
	private void pass2() {
		int hRecordIndex = -1;		//���� ������ codeList idx�� ����Ŵ
		for (int currentSection = 0; currentSection < tokenList.size(); currentSection++) {
			TokenTable tmpTokenTab = tokenList.get(currentSection);
			SymbolTable symTab = symtabList.get(currentSection);
			LiteralTable litTab = litTabList.get(currentSection);

			boolean ltorgFlag = false;
			StringBuilder tRecord = null;
			int tRecordStartAddr = -1;
			int tRecordLength = 0;

			int startAddr = 0;
			int endAddr = 0;

			for (int i = 0; i < tmpTokenTab.tokenList.size(); i++) {
				tmpTokenTab.makeObjectCode(i);
				Token token = tmpTokenTab.tokenList.get(i);

				//4��

				// START -> H record
				if ("START".equals(token.operator)) {
					StringBuilder hRecord = new StringBuilder("H");
					hRecord.append(String.format("%-6s", token.label));
					hRecord.append(String.format("%06X", token.location));
					hRecordIndex = codeList.size();
					codeList.add(hRecord.toString());
					startAddr = token.location;
					continue;
				}
				//LTORG
				if("LTORG".equals(token.operator)) ltorgFlag = true;

				// EXTDEF -> D record
				if ("EXTDEF".equals(token.operator)) {
					StringBuilder dRecord = new StringBuilder("D");
					for (int j = 0; j < TokenTable.MAX_OPERAND; j++) {
						String sym = token.operand[j];
						if (sym != null) {
							int addr = symTab.searchSymbol(sym);
							dRecord.append(String.format("%-6s%06X", sym, addr));
						}
					}
					codeList.add(dRecord.toString());
					continue;
				}

				// EXTREF -> R record
				if ("EXTREF".equals(token.operator)) {
					StringBuilder rRecord = new StringBuilder("R");
					for (int j = 0; j < TokenTable.MAX_OPERAND; j++) {
						String sym = token.operand[j];
						if (sym != null) {
							rRecord.append(String.format("%-6s", sym));
						}
					}
					codeList.add(rRecord.toString());
					continue;
				}

				// RESB/RESW ������ T���ڵ� flush
				if ("RESW".equals(token.operator) || "RESB".equals(token.operator)) {
					if (tRecord != null && tRecordLength > 0) {
						tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
						codeList.add(tRecord.toString());
						tRecord = null;
						tRecordStartAddr = -1;
						tRecordLength = 0;
					}
					continue;
				}
				//4������ �� �ܺ����� �ɺ��� �������� ������ ����
				if (token.operator !=null && token.operator.startsWith("+")) {
					boolean found = false;
					String symbol = token.operand[0];

					for (int k = 0; k < symtabList.size(); k++) {
						SymbolTable tmpSymTab = symtabList.get(k);
						if (tmpSymTab.searchSymbol(symbol) != -1) {
							found = true;
							break;
						}
					}

					if (!found) {
						throw new RuntimeException(String.format("�ܺνɺ� '%s' ��(��) � ���ǿ����� ã�� ���߽��ϴ�.", symbol));
					}
				}

				// object code �� T ���ڵ忡 �߰�
				if (token.objectCode != null && token.objectCode.length() > 0) {
					int objLen = token.objectCode.length() / 2;

					if (tRecord == null || tRecordLength + objLen > 30) {
						if (tRecord != null && tRecordLength > 0) {
							tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
							codeList.add(tRecord.toString());
						}
						tRecord = new StringBuilder("T");
						tRecordStartAddr = token.location;
						tRecordLength = 0;
					}

					tRecord.append(token.objectCode);
					tRecordLength += objLen;
				}

				// CSECT or END ó��
				if ("CSECT".equals(token.operator) || "END".equals(token.operator)) {
					// ���ͷ� ó�� (���� T ���ڵ忡 �̾� ���̱�)
					litTab = litTabList.get(currentSection);
					for (int j = 0; j < litTab.literalList.size(); j++) {
						String value = litTab.valueList.get(j);
						int length = value.length() / 2;
						int address = litTab.locationList.get(j);
						if(!ltorgFlag)	endAddr += length;		//���ͷ���ŭ ��ü���� �߰�

						if (tRecord == null) {
							// ���� T���ڵ尡 ���ٸ� ���� ����
							tRecord = new StringBuilder("T");
							tRecordStartAddr = address;
							tRecordLength = 0;
						} else if (tRecordLength + length > 30) {
							// T���ڵ尡 �� á���� ���� flush
							tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
							codeList.add(tRecord.toString());
							// �� T���ڵ� ����
							tRecord = new StringBuilder("T");
							tRecordStartAddr = address;
							tRecordLength = 0;
						}

						// ���ͷ� object code �̾� ���̱�
						tRecord.append(value);
						tRecordLength += length;
					}
					//T record ������ġ, �ּ� ����
					if (tRecord != null && tRecordLength > 0) {
						tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
						codeList.add(tRecord.toString());
						tRecord = null;
						tRecordLength = 0;
					}

					endAddr += token.location;

					// H ���ڵ� ���� ����
					if (hRecordIndex != -1) {
						int progLen = endAddr - startAddr;
						String hRec = codeList.get(hRecordIndex);
						String updated = hRec.substring(0, 13) + String.format("%06X", progLen);
						codeList.set(hRecordIndex, updated);
					}

					// M ���ڵ� �߰�
					ArrayList<String> extRefs = extRefList.get(currentSection);
					for (Token tok : tmpTokenTab.tokenList) {
						if (tok.objectCode != null && tok.operator != null && !tok.operator.equals("EXTREF") && !tok.operator.equals("EQU")) {
							for (int j = 0; j < TokenTable.MAX_OPERAND; j++) {
								String operand = tok.operand[j];
								if (operand == null) continue;
								String cleanOperand = operand.replaceAll("[#@]", "").split(",")[0];
								if (extRefs.contains(cleanOperand)) {
									int modLoc = tok.location + 1;
									codeList.add(String.format("M%06X05+%-6s", modLoc, cleanOperand));
								}
								//-��ȣ ����
								if(operand.contains("-")){
									String[] parts = operand.split("-");
									codeList.add(String.format("M%06X06+%-6s",tok.location, parts[0]));
									codeList.add(String.format("M%06X06-%-6s",tok.location, parts[1]));
								}
							}
						}
					}

					// E ���ڵ� �߰�
					if (currentSection == 0) {
						codeList.add("E" + String.format("%06X", startAddr)+"\n");
					} else {
						codeList.add("E\n");
					}

					// ���� ������ H ���ڵ� ����
					if (!"END".equals(token.operator) && token.label != null) {
						StringBuilder hRecord = new StringBuilder("H");
						hRecord.append(String.format("%-6s", token.label));
						hRecord.append("000000");
						hRecordIndex = codeList.size();
						codeList.add(hRecord.toString());
						startAddr = token.location;
					}
				}
			}

		}
	}


	/**
	 * �ۼ��� codeList�� ������¿� �°� ����Ѵ�.<br>
	 * @param fileName : ����Ǵ� ���� �̸�
	 */
	private void printObjectCode(String fileName) {
		try (PrintWriter writer = new PrintWriter(fileName)) {
			for (String line : codeList) {
				writer.println(line);
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
