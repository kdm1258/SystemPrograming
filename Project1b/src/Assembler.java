import javax.naming.ldap.ExtendedRequest;
import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;


/**
 * Assembler : 
 * 이 프로그램은 SIC/XE 머신을 위한 Assembler 프로그램의 메인 루틴이다.
 * 프로그램의 수행 작업은 다음과 같다. <br>
 * 1) 처음 시작하면 Instruction 명세를 읽어들여서 assembler를 세팅한다. <br>
 * 2) 사용자가 작성한 input 파일을 읽어들인 후 저장한다. <br>
 * 3) input 파일의 문장들을 단어별로 분할하고 의미를 파악해서 정리한다. (pass1) <br>
 * 4) 분석된 내용을 바탕으로 컴퓨터가 사용할 수 있는 object code를 생성한다. (pass2) <br>
 * 
 * <br><br>
 * 작성중의 유의사항 : <br>
 *  1) 새로운 클래스, 새로운 변수, 새로운 함수 선언은 얼마든지 허용됨. 단, 기존의 변수와 함수들을 삭제하거나 완전히 대체하는 것은 안된다.<br>
 *  2) 마찬가지로 작성된 코드를 삭제하지 않으면 필요에 따라 예외처리, 인터페이스 또는 상속 사용 또한 허용됨.<br>
 *  3) 모든 void 타입의 리턴값은 유저의 필요에 따라 다른 리턴 타입으로 변경 가능.<br>
 *  4) 파일, 또는 콘솔창에 한글을 출력시키지 말 것. (채점상의 이유. 주석에 포함된 한글은 상관 없음)<br>
 * 
 * <br><br>
 *  + 제공하는 프로그램 구조의 개선방법을 제안하고 싶은 분들은 보고서의 결론 뒷부분에 첨부 바랍니다. 내용에 따라 가산점이 있을 수 있습니다.
 */
public class Assembler {
	/** instruction 명세를 저장한 공간 */
	InstTable instTable;
	/** 읽어들인 input 파일의 내용을 한 줄 씩 저장하는 공간. */
	ArrayList<String> lineList;
	/** 프로그램의 section별로 symbol table을 저장하는 공간*/
	ArrayList<SymbolTable> symtabList;
	/** 프로그램의 section별로 프로그램을 저장하는 공간*/
	ArrayList<TokenTable> tokenList;
	/** 프로그램의 section별로 리터럴을 저장하는 공간*/
	ArrayList<LiteralTable> litTabList;
	/**
	 * Token, 또는 지시어에 따라 만들어진 오브젝트 코드들을 출력 형태로 저장하는 공간. <br>
	 * 필요한 경우 String 대신 별도의 클래스를 선언하여 ArrayList를 교체해도 무방함.
	 */
	ArrayList<String> codeList;

	/**exter요소들을 관리하는 리스트*/
	ArrayList<Map<String, Integer>> extDefList;
	ArrayList<ArrayList<String>> extRefList;

	/**
	 * 클래스 초기화. instruction Table을 초기화와 동시에 세팅한다.
	 * 
	 * @param instFile : instruction 명세를 작성한 파일 이름. 
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
	 * 어셈블러의 메인 루틴
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
	 * inputFile을 읽어들여서 lineList에 저장한다.<br>
	 * @param inputFile : input 파일 이름.
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
	 * 작성된 SymbolTable들을 출력형태에 맞게 출력한다.
	 * @param fileName : 저장되는 파일 이름
	 */
	private void printSymbolTable(String fileName) {
		try (PrintWriter writer = new PrintWriter(fileName)) {
			for (int section = 0; section < symtabList.size(); section++) {
				SymbolTable symTab = symtabList.get(section);

				for (int i = 0; i < symTab.symbolList.size(); i++) {
					String symbol = symTab.symbolList.get(i);
					int address = symTab.locationList.get(i);
					writer.printf("%-10s\t%X\n", symbol, address);  // 16진수 4자리 출력
				}

				writer.println();
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * 작성된 LiteralTable들을 출력형태에 맞게 출력한다.<br>
	 * @param fileName : 저장되는 파일 이름
	 */
	private void printLiteralTable(String fileName) {
		try (PrintWriter writer = new PrintWriter(fileName)) {
			for (int j = 0; j < litTabList.size(); j++) {
				LiteralTable litTab = litTabList.get(j);
				for (int i = 0; i < litTab.literalList.size(); i++) {
					String literal = litTab.literalList.get(i);
					int address = litTab.locationList.get(i);

					// =C'EOF' → EOF 로 출력
					String cleanLiteral = literal;
					if (literal.startsWith("=C'") && literal.endsWith("'")) {
						cleanLiteral = literal.substring(3, literal.length() - 1);
					} else if (literal.startsWith("=X'") && literal.endsWith("'")) {
						cleanLiteral = literal.substring(3, literal.length() - 1);
					} else if (literal.matches("=\\d+")) {
						cleanLiteral = literal.substring(1); // =5 → 5
					}

					writer.printf("%-10s\t%X\n", cleanLiteral, address);
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}


	/** 
	 * pass1 과정을 수행한다.<br>
	 *   1) 프로그램 소스를 스캔하여 토큰단위로 분리한 뒤 토큰테이블 생성<br>
	 *   2) label을 symbolTable에 정리<br>
	 *   <br><br>
	 *    주의사항 : SymbolTable과 TokenTable은 프로그램의 section별로 하나씩 선언되어야 한다.
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

			if (line.startsWith(".")) continue; // 주석 라인 무시

			// START 처리
			if (!startEncountered && "START".equals(token.operator)) {
				if (token.operand[0] != null)
					locctr = Integer.parseInt(token.operand[0], 16);
				token.location = locctr;
				startEncountered = true;
				if (token.label != null)
					symTab.putSymbol(token.label, locctr);
				continue;
			}

			// CSECT 처리
			if ("CSECT".equals(token.operator)) {
				locctr = litTab.assignAddresses(locctr);
				token.location = locctr;

				//기존 table을 이전 섹터에 저장
				symtabList.add(symTab);
				tokenList.add(tokTab);
				litTabList.add(litTab);

				//새로운 섹션 table 구성
				symTab = new SymbolTable();
				litTab = new LiteralTable();
				tokTab = new TokenTable(symTab, instTable, litTab);
				locctr = 0;

				symTab.putSymbol(token.label, locctr);

				continue;
			}

			// 외부 참조/정의이므로 locctr 증가 없음
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
						curExtDefMap.put(token.operand[i], -1); // 나중에 주소 채움
					}
				}
				extDefList.add(curExtDefMap);
				continue;
			}

			// EQU 처리
			if ("EQU".equals(token.operator)) {
				int value = 0;
				// 피연산자가 *인경우 현재 locctr할당
				if ("*".equals(token.operand[0])) {
					value = locctr;
				}
				//피연산자가 숫자인 경우
				else if (token.operand[0].matches("[0-9]+")) {
					value = Integer.parseInt(token.operand[0]);
				}
				//연산이 포함된 경우 ("-"연산)
				else if (token.operand[0].contains("-")) {
					String[] parts = token.operand[0].split("-");
					int left = symTab.searchSymbol(parts[0]);
					int right = symTab.searchSymbol(parts[1]);
					value = left - right;
				}
				//피연산자가 심볼
				else {
					value = symTab.searchSymbol(token.operand[0]) == -1 ? 0 : symTab.searchSymbol(token.operand[0]);
				}
				if (token.label != null)
					symTab.putSymbol(token.label, value);
				token.location = locctr;
				continue;
			}

			// LTORG 처리
			if ("LTORG".equals(token.operator)) {
				locctr = litTab.assignAddresses(locctr);
				continue;
			}

			// 리터럴 발견
			if (token.operand[0] != null && token.operand[0].startsWith("=")) {
				litTab.putLiteral(token.operand[0]);
			}

			// label 저장
			if (token.label != null)
				symTab.putSymbol(token.label, locctr);

			// 주소 지정
			token.location = locctr;

			// byteSize 계산
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

		// 마지막 LTORG 처리
		locctr = litTab.assignAddresses(locctr);

		// 테이블 저장
		symtabList.add(symTab);
		tokenList.add(tokTab);
		litTabList.add(litTab);
	}



	/**
	 * pass2 과정을 수행한다.<br>
	 *   1) 분석된 내용을 바탕으로 object code를 생성하여 codeList에 저장.
	 */
	private void pass2() {
		int hRecordIndex = -1;		//시작 섹터의 codeList idx를 가리킴
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

				//4혀

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

				// RESB/RESW 만나면 T레코드 flush
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
				//4형식일 때 외부참조 심볼이 존재하지 않으면 에러
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
						throw new RuntimeException(String.format("외부심볼 '%s' 을(를) 어떤 섹션에서도 찾지 못했습니다.", symbol));
					}
				}

				// object code → T 레코드에 추가
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

				// CSECT or END 처리
				if ("CSECT".equals(token.operator) || "END".equals(token.operator)) {
					// 리터럴 처리 (기존 T 레코드에 이어 붙이기)
					litTab = litTabList.get(currentSection);
					for (int j = 0; j < litTab.literalList.size(); j++) {
						String value = litTab.valueList.get(j);
						int length = value.length() / 2;
						int address = litTab.locationList.get(j);
						if(!ltorgFlag)	endAddr += length;		//리터럴만큼 전체길이 추가

						if (tRecord == null) {
							// 아직 T레코드가 없다면 새로 시작
							tRecord = new StringBuilder("T");
							tRecordStartAddr = address;
							tRecordLength = 0;
						} else if (tRecordLength + length > 30) {
							// T레코드가 꽉 찼으면 먼저 flush
							tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
							codeList.add(tRecord.toString());
							// 새 T레코드 시작
							tRecord = new StringBuilder("T");
							tRecordStartAddr = address;
							tRecordLength = 0;
						}

						// 리터럴 object code 이어 붙이기
						tRecord.append(value);
						tRecordLength += length;
					}
					//T record 시작위치, 주소 삽입
					if (tRecord != null && tRecordLength > 0) {
						tRecord.insert(1, String.format("%06X%02X", tRecordStartAddr, tRecordLength));
						codeList.add(tRecord.toString());
						tRecord = null;
						tRecordLength = 0;
					}

					endAddr += token.location;

					// H 레코드 길이 갱신
					if (hRecordIndex != -1) {
						int progLen = endAddr - startAddr;
						String hRec = codeList.get(hRecordIndex);
						String updated = hRec.substring(0, 13) + String.format("%06X", progLen);
						codeList.set(hRecordIndex, updated);
					}

					// M 레코드 추가
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
								//-기호 포함
								if(operand.contains("-")){
									String[] parts = operand.split("-");
									codeList.add(String.format("M%06X06+%-6s",tok.location, parts[0]));
									codeList.add(String.format("M%06X06-%-6s",tok.location, parts[1]));
								}
							}
						}
					}

					// E 레코드 추가
					if (currentSection == 0) {
						codeList.add("E" + String.format("%06X", startAddr)+"\n");
					} else {
						codeList.add("E\n");
					}

					// 다음 섹션의 H 레코드 시작
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
	 * 작성된 codeList를 출력형태에 맞게 출력한다.<br>
	 * @param fileName : 저장되는 파일 이름
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
