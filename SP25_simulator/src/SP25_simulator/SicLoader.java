package SP25_simulator;

import java.io.*;
import java.util.ArrayList;

/**
 * SicLoader는 프로그램을 해석해서 메모리에 올리는 역할을 수행한다. 이 과정에서 linker의 역할 또한 수행한다.
 *
 * SicLoader가 수행하는 일을 예를 들면 다음과 같다. - program code를 메모리에 적재시키기 - 주어진 공간만큼 메모리에 빈
 * 공간 할당하기 - 과정에서 발생하는 symbol, 프로그램 시작주소, control section 등 실행을 위한 정보 생성 및 관리
 */
public class SicLoader {
    ResourceManager rMgr;
    SymbolTable symbolTable;
    ArrayList<MRecordInfo> modiInfoList = new ArrayList<>();
    ArrayList<InstructionDisplayData> instructionDisplayList = new ArrayList<>(); // 변수명 변경 및 타입 변경

    // 화면 표시용 명령어 정보를 담을 내부 클래스 (또는 별도 public 클래스)
    public static class InstructionDisplayData {
        public final int address;
        public final String mnemonic;
        public byte[] machineCode;
        public final int length;
        public final boolean isFormat4;

        public InstructionDisplayData(int address, String mnemonic, byte[] machineCode, int length, boolean isFormat4) {
            this.address = address;
            this.mnemonic = mnemonic;
            this.machineCode = machineCode;
            this.length = length;
            this.isFormat4 = isFormat4;
        }
    }

    //수정레코드 정보 저장
    public class MRecordInfo{
        int targetAddr;
        String targetSymbol;
        char operator;
        int length;

        public MRecordInfo(int targetAddr, String targetSymbol, char operator, int length){
            this.targetAddr = targetAddr;
            this.targetSymbol = targetSymbol;
            this.operator = operator;
            this.length = length;
        }
    }

    public SicLoader(ResourceManager resourceManager) {
        // 필요하다면 초기화
        setResourceManager(resourceManager);

        //symboltable 링크
        if (this.rMgr != null) {
            this.symbolTable = this.rMgr.getSymbolTable();
            this.instructionDisplayList = new ArrayList<>();
            rMgr.setInstructionDisplayList(this.instructionDisplayList); // ResourceManager에도 넘겨줌
        }

    }

    /**
     * Loader와 프로그램을 적재할 메모리를 연결시킨다.
     *
     * @param resourceManager
     */
    public void setResourceManager(ResourceManager resourceManager) {
        this.rMgr = resourceManager;
    }

    /**
     * object code를 읽어서 load과정을 수행한다. load한 데이터는 resourceManager가 관리하는 메모리에 올라가도록
     * 한다. load과정에서 만들어진 symbol table 등 자료구조 역시 resourceManager에 전달한다.
     *
     * @param objectCode 읽어들인 파일
     */
    public void load(File objectCode) {
        //H레코드
        String programName = "";
        int startAddress = 0;
        int programLength = 0;
        int sectionByte = 0;

        // 파일 로드 시 이전 정보 클리어
        if (instructionDisplayList != null) {
            instructionDisplayList.clear();
        } else {
            instructionDisplayList = new ArrayList<>();
        }
        // ResourceManager의 리스트도 동일한 인스턴스를 참조하도록 설정
        if (rMgr != null) {
            rMgr.setInstructionDisplayList(instructionDisplayList);
        }

        try (BufferedReader reader = new BufferedReader(new FileReader(objectCode))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isEmpty()) continue;

                char recordType = line.charAt(0);               //레코드 종류 빼놓기 ex)H, T, E ...
                String recordContent = line.substring(1);

                switch (recordType) {
                    case 'H': // Header Record
                        programName = recordContent.substring(0, 6).trim();
                        startAddress = Integer.parseInt(recordContent.substring(6, 12), 16);
                        programLength = Integer.parseInt(recordContent.substring(12, 18), 16);
                        symbolTable.putSymbol(programName, startAddress+sectionByte);       //시작 프로그램정보 symtab 저장

                        if(rMgr.programName == null || rMgr.programName.isEmpty()) {
                            // ResourceManager에 프로그램 기본 정보 설정
                            rMgr.setProgramName(programName);
                            rMgr.setProgramStartAddress(startAddress);
                        }
                        rMgr.setProgramLength(programLength + sectionByte); //전체길이니까 따로 빼서 처리
                        break;

                    case 'T': // Text Record
                        int startAddr = Integer.parseInt(recordContent.substring(0, 6), 16);
                        int lengthBytes = Integer.parseInt(recordContent.substring(6, 8), 16);
                        String tRecObjectCode = recordContent.substring(8);

                        // T 레코드 내용이 메모리에 적재될 실제 시작 주소
                        int currentMemAddr = startAddr; // 현재 명령어의 시작 주소 (T레코드 내에서)

                        for (int i = 0; i < tRecObjectCode.length() && (i / 2) < lengthBytes; /* i는 내부에서 증가 */ ) {
                            if (i + 2 > tRecObjectCode.length()) break;
                            String firstByteHex = tRecObjectCode.substring(i, i + 2);
                            int firstByteValue = Integer.parseInt(firstByteHex, 16);

                            int baseOpcode = firstByteValue & 0xFC;
                            int format = SicSimulator.OpcodeTable.getFormat(baseOpcode);
                            String mnemonic = SicSimulator.OpcodeTable.getOper(baseOpcode);
                            int instLenBytes = format;
                            boolean isF4 = false;

                            if (format == 0) {
                                instLenBytes = 1;
                                mnemonic = (mnemonic == null || mnemonic.equalsIgnoreCase("UNK")) ? "" : mnemonic;
                            } else if (format == 3) {
                                int byteOffset = i / 2;
                                if ((i + 4 <= tRecObjectCode.length()) && (byteOffset + 1 < lengthBytes)) {
                                    String secondByteHex = tRecObjectCode.substring(i + 2, i + 4);
                                    int secondByteValue = Integer.parseInt(secondByteHex, 16);
                                    if ((secondByteValue & 0x10) != 0) { isF4 = true; instLenBytes = 4; }
                                }
                            }

                            int remainChars = tRecObjectCode.length() - i;
                            int remainBytes = remainChars / 2;
                            int remainingBytesInTRec = lengthBytes - (i / 2);
                            if (instLenBytes > remainBytes) instLenBytes = remainBytes;
                            if (instLenBytes > remainingBytesInTRec) instLenBytes = remainingBytesInTRec;
                            if (instLenBytes <= 0) break;
                            if (isF4 && instLenBytes < 4) isF4 = false;
                            if (format == 3 && !isF4 && instLenBytes < 3 && !mnemonic.endsWith("?")) mnemonic += "?";

                            byte[] currentInstMachineCode = new byte[instLenBytes];
                            boolean writeErr = false;
                            for (int k = 0; k < instLenBytes; k++) {
                                String byteHex = tRecObjectCode.substring(i + (k * 2), i + (k * 2) + 2);
                                int byteVal = Integer.parseInt(byteHex, 16);
                                currentInstMachineCode[k] = (byte) byteVal;
                                if (currentMemAddr + k < rMgr.memory.length) {
                                    rMgr.writeMemory(sectionByte + currentMemAddr + k, byteVal);
                                } else {
                                    System.err.println("Loader T-REC Error: Memory OOB. Addr: " + (currentMemAddr + k));
                                    byte[] temp = new byte[k]; System.arraycopy(currentInstMachineCode,0,temp,0,k);
                                    currentInstMachineCode = temp; instLenBytes = k;
                                    if(isF4 && instLenBytes < 4) isF4 = false;
                                    writeErr = true; break;
                                }
                            }
                            if(writeErr && instLenBytes == 0) break;

                            InstructionDisplayData instData = new InstructionDisplayData(
                                    sectionByte+currentMemAddr, mnemonic, currentInstMachineCode,
                                    instLenBytes, isF4);
                            this.instructionDisplayList.add(instData); // 리스트에 추가!

                            i += instLenBytes * 2;
                            currentMemAddr += instLenBytes;
                        }
                        break;

                    case 'M': // Modification Record (SIC/XE에서 사용)
                        // 이 레코드는 주로 주소 수정이나 프로그램 링크 시 사용됩니다.
                        int targetAddr = Integer.parseInt(recordContent.substring(0,6),16);
                        int length = Integer.parseInt(recordContent.substring(6,8));
                        char operator = recordContent.charAt(8);
                        String targetSymbol = recordContent.substring(9);
                        modiInfoList.add(new MRecordInfo(targetAddr+sectionByte,targetSymbol, operator, length));
                        break;

                    case 'E': // End Record
                        sectionByte += programLength;
                        if (recordContent.length() >= 6) {
                            int exeStartAddress = Integer.parseInt(recordContent.substring(0, 6), 16);
                            rMgr.setExeAddr(exeStartAddress); // 실행 시작 주소 설정
                        }
                        break;

                    case 'R': // Refer Record (외부 참조 심볼, SIC/XE에서 사용)
                        break;
                    case 'D': // Define Record (내부 정의 심볼, SIC/XE에서 사용)
                        // D 레코드는 프로그램이 정의하는 심볼, 주소 symbolTable에 저장
                        for(int i = 0 ; i < recordContent.length() ; i+=12){
                            String symbolName = recordContent.substring(i,i+6);
                            int symbolAddr = Integer.parseInt(recordContent.substring(i+6, i+12), 16);
                            symbolTable.putSymbol(symbolName, symbolAddr+sectionByte);
                        }
                        break;

                    default:
                        System.err.println("Unknown record type: " + recordType + " in line: " + line);
                        break;
                }
            }
        } catch (IOException e) {
            System.err.println("Error reading object code file: " + e.getMessage());
            e.printStackTrace();
        } catch (NumberFormatException e) {
            System.err.println("Error parsing hexadecimal value in object code: " + e.getMessage());
            e.printStackTrace();
        }

        for (MRecordInfo modi : modiInfoList) {
            int modificationAddress = modi.targetAddr; // MRecordInfo 생성 시 이미 절대 주소로 계산됨
            char operator = modi.operator;
            int lengthInHalfBytes = modi.length; // 수정할 필드의 길이 (half-byte 단위)
            String symbolName = modi.targetSymbol;

            int symbolValue = symbolTable.search(symbolName);

            //  수정할 필드가 차지하는 바이트 수 계산
            int numBytesToModify = (lengthInHalfBytes + 1) / 2;

            // 메모리에서 현재 필드 값을 읽어오기
            byte[] currentMemoryBytes = rMgr.getMemory(modificationAddress, numBytesToModify);
            long originalFieldValue = 0; // 필드 값을 long으로 처리하여 오버플로우 방지

            long modifiedFieldValue = 0;
            boolean modificationSuccess = true;

            if (lengthInHalfBytes == 5) { // Format 4 명령어의 20비트 주소 필드
                // 현재 20비트 값: 첫 바이트의 하위 4비트 + 다음 2바이트
                originalFieldValue = ((long) (currentMemoryBytes[0] & 0x0F) << 16) |
                        ((long) (currentMemoryBytes[1] & 0xFF) << 8) |
                        ((long) (currentMemoryBytes[2] & 0xFF));

                if (operator == '+') {
                    modifiedFieldValue = originalFieldValue + symbolValue;
                } else if (operator == '-') {
                    modifiedFieldValue = originalFieldValue - symbolValue;
                } else {
                    modificationSuccess = false;
                }
                modifiedFieldValue &= 0x0FFFFF; // 20비트 마스크

                if (modificationSuccess) {
                    // 수정된 값을 바이트 배열에 다시 반영 (첫 바이트의 상위 4비트는 유지)
                    currentMemoryBytes[0] = (byte) ((currentMemoryBytes[0] & 0xF0) | ((modifiedFieldValue >> 16) & 0x0F));
                    currentMemoryBytes[1] = (byte) ((modifiedFieldValue >> 8) & 0xFF);
                    currentMemoryBytes[2] = (byte) (modifiedFieldValue & 0xFF);
                }

            } else if (lengthInHalfBytes == 6) { // SIC WORD (24비트 전체)
                // 현재 24비트 값
                originalFieldValue = (((long) currentMemoryBytes[0] & 0xFF) << 16) |
                        (((long) currentMemoryBytes[1] & 0xFF) << 8) |
                        ((long) currentMemoryBytes[2] & 0xFF);
                // 24비트 음수 표현을 위해 부호 확장 (int로 변환 시 byteToInt에서 처리되지만, 여기서도 명시 가능)
                if ((originalFieldValue & 0x00800000L) != 0) { // 최상위 비트가 1이면
                    originalFieldValue |= 0xFFFFFFFFFF000000L; // long 타입으로 부호 확장
                }

                if (operator == '+') {
                    modifiedFieldValue = originalFieldValue + symbolValue;
                } else if (operator == '-') {
                    modifiedFieldValue = originalFieldValue - symbolValue;
                } else {
                    modificationSuccess = false;
                }
                modifiedFieldValue &= 0xFFFFFFL; // 24비트 마스크 (long의 하위 24비트)

                if (modificationSuccess) {
                    currentMemoryBytes[0] = (byte) ((modifiedFieldValue >> 16) & 0xFF);
                    currentMemoryBytes[1] = (byte) ((modifiedFieldValue >> 8) & 0xFF);
                    currentMemoryBytes[2] = (byte) (modifiedFieldValue & 0xFF);
                }
            }

            //연산자가 유효했고, 지원되는 길이었다면 수정된 바이트를 메모리에 다시 쓰기
            if (modificationSuccess) {
                rMgr.setMemory(modificationAddress, currentMemoryBytes, numBytesToModify);
            }
        }
        buildInstList();
    }

    /**
     * 메모리 수정 후, 최종 메모리 상태를 바탕으로 instructionDisplayList를 생성합니다.
     */
    private void buildInstList() {
        if (rMgr == null || this.instructionDisplayList == null || rMgr.programStartAddr == -1 || rMgr.programLength == 0) {
            // 준비안되었으면 리턴
            return;
        }
        for(InstructionDisplayData inst : instructionDisplayList){
            int addr = inst.address;
            byte[] tmp = rMgr.getMemory(addr,inst.length);
            if(!tmp.equals(inst.machineCode)){
                inst.machineCode = tmp;
            }
        }
    }
}

