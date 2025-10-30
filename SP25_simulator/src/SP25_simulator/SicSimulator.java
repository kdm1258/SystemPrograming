package SP25_simulator;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * 시뮬레이터로서의 작업을 담당한다. VisualSimulator에서 사용자의 요청을 받으면 이에 따라 ResourceManager에 접근하여
 * 작업을 수행한다.
 *
 * 작성중의 유의사항 : 1) 새로운 클래스, 새로운 변수, 새로운 함수 선언은 얼마든지 허용됨. 단, 기존의 변수와 함수들을 삭제하거나
 * 완전히 대체하는 것은 지양할 것. 2) 필요에 따라 예외처리, 인터페이스 또는 상속 사용 또한 허용됨. 3) 모든 void 타입의 리턴값은
 * 유저의 필요에 따라 다른 리턴 타입으로 변경 가능. 4) 파일, 또는 콘솔창에 한글을 출력시키지 말 것. (채점상의 이유. 주석에 포함된
 * 한글은 상관 없음)
 *
 *
 *
 * + 제공하는 프로그램 구조의 개선방법을 제안하고 싶은 분들은 보고서의 결론 뒷부분에 첨부 바랍니다. 내용에 따라 가산점이 있을 수
 * 있습니다.
 */
public class SicSimulator {
    ResourceManager rMgr;
    InstLuncher instLuncher;
    private byte[] lastExeInstBytes;    //마지막에 실행한 inst opcode
    private int targetAddress;          //얘는 gui로 ta넘길때씀


    public SicSimulator(ResourceManager resourceManager) {
        // 필요하다면 초기화 과정 추가
        this.rMgr = resourceManager;
        this.instLuncher = new InstLuncher(rMgr);
    }

    /**
     * 레지스터, 메모리 초기화 등 프로그램 load와 관련된 작업 수행. 단, object code의 메모리 적재 및 해석은
     * SicLoader에서 수행하도록 한다.
     */
    public void load(File program) {
        /* 메모리 초기화, 레지스터 초기화 등 */
        if (rMgr == null) {
            addLog("오류: ResourceManager가 초기화되지 않았습니다.");
            return;
        }

        if (rMgr.exeAddr != -1) { // E 레코드에 명시된 실행 시작 주소
            rMgr.setRegister(ResourceManager.REG_PC, rMgr.exeAddr);
        } else if (rMgr.programStartAddr != -1) { // 대안으로 H 레코드의 프로그램 시작 주소
            rMgr.setRegister(ResourceManager.REG_PC, rMgr.programStartAddr);
            addLog("정보: E 레코드 실행 시작 주소 없음. PC를 프로그램 시작 주소로 설정.");
        } else {
            addLog("오류: PC를 설정할 수 없습니다. 프로그램 시작 및 실행 주소가 유효하지 않습니다.");
            // 실행 버튼 비활성화 등의 조치가 필요할 수 있음
            return;
        }

    }

    /**
     * 1개의 instruction이 수행된 모습을 보인다.
     * 프로그램 종료시 1리턴 아니면 0 리턴
     * (실제 sic xe머신에서는 이 프로그램을 호출한 다음 위치가 pc에 있어야하니까 이런 처리 필요없긴함)
     */
    public int oneStep() {
        if(rMgr == null || instLuncher == null){
            addLog("오류: ResourceManager 또는 InstLuncher가 초기화되지 않았습니다.");
            return 0;
        }

        int currentPC = rMgr.getRegister(rMgr.REG_PC);      //아직까진 이전 PC
        int originalPC = currentPC;

        //메모리초과 처리
        if(currentPC<0 || currentPC > rMgr.memory.length){
            this.addLog(String.format("오류: PC (0x%06X)가 메모리 범위를 벗어났습니다. 실행 중단.", originalPC));
            return 0;
        }

        //1바이트 가져오기
        byte byte1 = rMgr.getMemory(currentPC, 1)[0];
        int opcodeFull = byte1 & 0xFF;      // 첫 바이트 전체 (opcode + ni)
        int opcode = opcodeFull & 0xFC; // ni 비트 제외한 순수 opcode

        String mnemonic = OpcodeTable.getOper(opcode);
        int opFormat = OpcodeTable.getFormat(opcode);
        int opLength = opFormat;    //일단 opFormat과 같음 4형식일때 +1이된다
        boolean isFormat4 = false;

        if(OpcodeTable.getFormat(opcode)==3){
            if(currentPC + 1 < rMgr.memory.length) {
                byte byte2 = rMgr.getMemory(currentPC + 1, 1)[0];
                if ((byte2 & 0x10) != 0) {    //e flag 확인
                    isFormat4 = true;
                    opLength++;
                }
            }
            else{
                this.addLog("오류 : 메모리 범위 초과");
            }
        }

        if (currentPC + opLength > rMgr.memory.length) {
            this.addLog(String.format("오류: 전체 명령어(%s, %d bytes) 인출 중 메모리 범위 초과 (PC: 0x%06X)", mnemonic, opLength, currentPC));
            return 0;
        }
        byte[] instructionBytes = new byte[opLength];
        instructionBytes = rMgr.getMemory(currentPC,opLength);

        rMgr.setRegister(rMgr.REG_PC, currentPC+opLength);  //다음명령어를 가리키게 pc 업데이트
        currentPC += opLength;

        //flag 설정
        boolean nFlag = (opcodeFull & 0x02) != 0;
        boolean iFlag = (opcodeFull & 0x01) != 0;
        boolean xFlag = false, bFlag = false, pFlag = false; // eFlag는 isFormat4로 대체
        int dispOrAddr = 0; // Format 3/4의 변위/주소 또는 Format 2의 r1r2

        if (opFormat == 2) {
            dispOrAddr = instructionBytes[1] & 0xFF; // r1r2 바이트
        } else if (opFormat >= 3) { // Format 3 또는 4
            xFlag = (instructionBytes[1] & 0x80) != 0;
            bFlag = (instructionBytes[1] & 0x40) != 0;
            pFlag = (instructionBytes[1] & 0x20) != 0;

            if (isFormat4) { // Format 4 (20-bit address)
                dispOrAddr = ((instructionBytes[1] & 0x0F) << 16) |
                        ((instructionBytes[2] & 0xFF) << 8) |
                        (instructionBytes[3] & 0xFF);
            } else { // Format 3 (12-bit displacement)
                dispOrAddr = ((instructionBytes[1] & 0x0F) << 8) |
                        (instructionBytes[2] & 0xFF);
            }
        }

        //TA설정
        int targetAddress = -1;       // 메모리 참조 시 사용될 최종 유효 주소 (TA)
        boolean isOperandImmediate = false; // 피연산자가 immediate 값인지 여부

        // 1. 주소 지정 방식 결정 (n, i 비트 기준)
        if (iFlag && !nFlag) { // Immediate addressing (i=1, n=0)
            isOperandImmediate = true;
            targetAddress = dispOrAddr; // dispOrAddressField가 피연산자 값 자체가 됨.

        }
        else { // Not Immediate: Target Address 계산 필요
            isOperandImmediate = false;
            int calculatedTA = 0; // 인덱싱 및 간접 주소 지정 전의 기본 주소

            // 2. 기본 주소 계산 (Format, b, p 플래그 기준)
            if (isFormat4) { // Format 4 (e=1)
                calculatedTA = dispOrAddr; // dispOrAddressField는 20비트 절대 주소
            }
            else { // Format 3 (e=0)
                if (bFlag && !pFlag) { // Base relative (b=1, p=0)
                    // dispOrAddressField는 12비트 부호 없는 변위 (0~4095)
                    calculatedTA = rMgr.getRegister(ResourceManager.REG_B) + (dispOrAddr & 0xFFF);
                }
                else if (!bFlag && pFlag) { // PC relative (b=0, p=1)
                    int signedDisp = (dispOrAddr & 0x800) != 0 ? dispOrAddr | 0xFFFFF000 : dispOrAddr;
                    calculatedTA = currentPC + signedDisp;
                }

                else if (!bFlag && !pFlag) { // Direct addressing for Format 3 (b=0, p=0)
                    // dispOrAddressField는 12비트 절대 주소
                    // n,i 플래그에 따라 simple/indirect가 결정됨.
                    calculatedTA = dispOrAddr & 0xFFF; // 12비트 주소로 사용
                }
                else { // b=1, p=1 (Invalid or reserved for future use)
                    addLog(String.format("  오류: 잘못된 주소 지정 플래그 조합입니다 (b=1, p=1)."));
                    return 0; // oneStep() 함수 종료
                }
            }

            // 3. 인덱스 주소 지정 적용 (x=1)
            // (Immediate 모드가 아닐 때만 적용)
            if (xFlag) {
                int xRegisterValue = rMgr.getRegister(ResourceManager.REG_X);
                calculatedTA += xRegisterValue;
            }

            targetAddress = calculatedTA; // 인덱싱까지 적용된 주소 (간접 주소 지정 전의 값)

            if(opFormat != 1 && opFormat !=2){
                setTA(targetAddress);   //gui로 넘길준비
            }
            // 4. 간접 주소 지정 적용 (n=1, i=0)
            // (Immediate 모드가 아닐 때만 적용)
            if (nFlag && !iFlag) { // Indirect addressing
                if (targetAddress < 0 || targetAddress + 2 >= rMgr.memory.length) { // 3바이트(WORD) 읽기 확인
                    addLog(String.format("  오류: 간접 주소 참조(0x%06X) 시 메모리 범위를 벗어났습니다.",
                            targetAddress & 0xFFFFFF));
                    return 0;
                }
                byte[] finalTaBytes = rMgr.getMemory(targetAddress, 3); // 메모리에서 최종 TA(3바이트 워드)를 읽음
                targetAddress = rMgr.byteToInt(finalTaBytes);           // 바이트 배열을 정수(TA)로 변환
            }
        }

        switch (mnemonic){
            case "STL":
                instLuncher.stl(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "JSUB":
                instLuncher.jsub(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "CLEAR":
                instLuncher.clear(dispOrAddr);
                addLog(String.format("%s", mnemonic));
                break;
            case "LDT":
                instLuncher.ldt(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "TD":
                instLuncher.td(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "JEQ":
                instLuncher.jeq(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "RD":
                instLuncher.rd(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "COMPR":
                instLuncher.compr(dispOrAddr);
                addLog(String.format("%s", mnemonic));
                break;
            case "STCH":
                instLuncher.stch(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "TIXR":
                instLuncher.tixr(dispOrAddr);
                addLog(String.format("%s", mnemonic));
                break;
            case "JLT":
                instLuncher.jlt(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "STX":
                instLuncher.stx(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "RSUB":
                instLuncher.rsub();
                addLog(String.format("%s", mnemonic));
                break;
            case "LDA":
                instLuncher.lda(targetAddress, isOperandImmediate);
                addLog(String.format("%s", mnemonic));
                break;
            case "COMP":
                instLuncher.comp(targetAddress, isOperandImmediate);
                addLog(String.format("%s", mnemonic));
                break;
            case "LDCH":
                instLuncher.ldch(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "WD":
                instLuncher.wd(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            case "J":
                instLuncher.j(targetAddress);
                addLog(String.format("%s", mnemonic));
                //임시방편 프로그램 끝내기 코드
                if(nFlag & !iFlag) return 1;
                break;
            case "STA":
                instLuncher.sta(targetAddress);
                addLog(String.format("%s", mnemonic));
                break;
            default:
        }
        setLastExeInst(instructionBytes);
        return 0;
    }

    /**
     * 남은 모든 instruction이 수행된 모습을 보인다.
     */
    //근데 gui업데이트 때문에 그냥 visualSimulator에서 구현한거 돌릴듯
    public void allStep() {
        while (true) {
            int currentPC = rMgr.getRegister(ResourceManager.REG_PC);

            // 종료 조건 1: 메모리 범위를 벗어남
            if (currentPC < 0 || currentPC >= rMgr.programLength) {
                addLog("프로그램 종료: PC가 메모리 범위를 벗어남");
                break;
            }
            // 명령어 한 줄 실행
            oneStep();
        }
    }


    /**
     * 각 단계를 수행할 때 마다 관련된 기록을 남기도록 한다.
     */
    public void addLog(String log) {
        if (rMgr != null) {
            rMgr.addRuntimeLog(log);
        } else {
            System.out.println("SIM_LOG (rMgr is null): " + log); // ResourceManager가 없는 비상 상황
        }
    }

    //마지막에 실행된 instruction opcode를 gui에 전달
    public byte[] getLastExeInst() {
        byte bytes[] = this.lastExeInstBytes;
        this.lastExeInstBytes = null;
        return bytes;
    }

    public void setLastExeInst(byte[] bytes){
        this.lastExeInstBytes = (bytes != null) ? bytes.clone() : null;
    }

    public int getTA() {
        int targetAddress = this.targetAddress;
        this.targetAddress = 0;  //한번 넘기고 초기화
        return targetAddress;
    }

    public void setTA(int targetAddress){
        this.targetAddress = targetAddress;
    }

    /**
     * opcode 정보를 저장해 실행할 명령어 비교
     */
    public static class OpcodeTable {
        // Key: 기본 Opcode (ni 비트 제외), Value: {형식, 니모닉} (형식 0은 오류/미정의)
        private static final Map<Integer, Object[]> opTable = new HashMap<>();
        static {
            // Format 3/4 (길이는 e비트에 따라 달라짐)
            opTable.put(0x18, new Object[]{3, "ADD"});
            opTable.put(0x00, new Object[]{3, "LDA"});
            opTable.put(0x0C, new Object[]{3, "STA"});
            opTable.put(0x4C, new Object[]{3, "RSUB"}); // 실제로는 Format 3이나, 길이가 고정적임
            opTable.put(0x3C, new Object[]{3, "J"});
            opTable.put(0x30, new Object[]{3, "JEQ"});
            opTable.put(0x38, new Object[]{3, "JLT"});
            opTable.put(0x14, new Object[]{3, "STL"});
            opTable.put(0x48, new Object[]{3, "JSUB"});
            opTable.put(0x28, new Object[]{3, "COMP"});
            opTable.put(0x74, new Object[]{3, "LDT"});
            opTable.put(0xE0, new Object[]{3, "TD"});
            opTable.put(0x54, new Object[]{3, "STCH"});
            opTable.put(0xDC, new Object[]{3, "WD"});
            opTable.put(0xD8, new Object[]{3, "RD"});
            opTable.put(0x10, new Object[]{3, "STX"});
            opTable.put(0x50, new Object[]{3, "LDCH"});
            opTable.put(0x44, new Object[]{3, "EOF"});     //symtable이 없으니 따로 처리
            // Format 2 (길이 2)
            opTable.put(0x90, new Object[]{2, "ADDR"});
            opTable.put(0xB4, new Object[]{2, "CLEAR"});
            opTable.put(0xA0, new Object[]{2, "COMPR"});
            opTable.put(0xB8, new Object[]{2, "TIXR"});
            // Format 1 (길이 1) - 예시 없음
        }

        public static int getFormat(int baseOpcode) {
            if (opTable.containsKey(baseOpcode)) {
                return (int) opTable.get(baseOpcode)[0];
            }
            return 0; // 알 수 없는 Opcode 또는 Format 0
        }

        public static String getOper(int baseOpcode) {
            if (opTable.containsKey(baseOpcode)) {
                return (String) opTable.get(baseOpcode)[1];
            }
            return "UNK"; // Unknown
        }
    }
}
