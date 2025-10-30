package SP25_simulator;

import java.io.*;
import java.util.*;

/**
 * ResourceManager는 컴퓨터의 가상 리소스들을 선언하고 관리하는 클래스입니다.
 * 메모리, 레지스터, 장치, 심볼 테이블, GUI 표시용 데이터 등을 포함합니다.
 */
public class ResourceManager {
    // --- 가상 하드웨어 리소스 ---
    public byte[] memory = new byte[65536]; // 64KB 메모리
    public int[] register = new int[10];    // 범용 레지스터 (A,X,L,PC,SW, B,S,T 등)
    public double register_F;               // F (부동소수점) 레지스터

    // --- 장치 관리 ---
    public HashMap<String, Object> deviceManager = new HashMap<>();

    // --- 로더 및 시뮬레이터 지원 자료구조 ---
    public SymbolTable symbolTable;
    public String programName;
    public int programStartAddr = -1; // 프로그램의 실제 메모리 시작 주소 (-1은 미설정 의미)
    public int programLength = 0;    // 프로그램 전체 길이
    public int exeAddr = -1;         // E 레코드에 명시된 실행 시작 주소 (-1은 미설정 의미)

    // --- GUI 표시용 데이터 ---
    // SicLoader가 채우고 VisualSimulator가 사용하는 명령어 정보 리스트
    private ArrayList<SicLoader.InstructionDisplayData> instructionDisplayListForGUI;
    // SicSimulator의 load()에서 더 이상 사용하지 않음 (런타임 로그는 runtimeLogBuffer 사용)
    private String disassembledMnemonicsForGUI;
    // SicLoader가 instructionDisplayListForGUI를 채우므로, 이 문자열은 더 이상 직접 사용되지 않을 가능성 높음
    private String machineCodeViewForGUI;
    // SicSimulator의 oneStep/allStep 실행 중 발생하는 로그를 위한 버퍼
    private StringBuilder runtimeLogBuffer = new StringBuilder();


    // 레지스터 번호 상수 (SIC/XE 표준)
    public static final int REG_A = 0;  // Accumulator
    public static final int REG_X = 1;  // Index register
    public static final int REG_L = 2;  // Linkage register
    public static final int REG_B = 3;  // Base register (XE)
    public static final int REG_S = 4;  // General purpose register (XE)
    public static final int REG_T = 5;  // General purpose register (XE)
    public static final int REG_F = 6;  // Floating-point accumulator (XE, 48 bits)
    public static final int REG_PC = 8; // Program Counter
    public static final int REG_SW = 9; // Status Word
    private String usingDevice = "0";   //현재 사용중인 디바이스 정보
    /**
     * ResourceManager 생성자.
     * 모든 리소스를 초기 상태로 설정합니다.
     */
    public ResourceManager() {
        initializeResource();
    }

    /**
     * 모든 가상 리소스 (메모리, 레지스터, 내부 자료구조 등)를 초기화합니다.
     * 새 파일을 로드하기 전에 호출됩니다.
     */
    public void initializeResource() {
        // 메모리 초기화 (0으로 채움)
        this.memory = new byte[65536];
        Arrays.fill(this.memory, (byte) 0);

        // 정수 레지스터 초기화 (0으로 채움)
        this.register = new int[10];
        Arrays.fill(this.register, 0);
        this.register_F = 0.0; // 부동소수점 레지스터 초기화

        // 장치 매니저 초기화 (기존 스트림이 있다면 닫아야 하지만, 여기서는 간단히 새로 생성)
        closeDevice(); // 기존 장치 연결 해제 및 deviceManager 클리어
        this.deviceManager = new HashMap<>();

        // 심볼 테이블 초기화
        this.symbolTable = new SymbolTable(); // 새 SymbolTable 인스턴스 생성

        // 프로그램 정보 초기화
        this.programName = null;
        this.programStartAddr = -1; // -1 등으로 미설정 상태 표시
        this.programLength = 0;
        this.exeAddr = -1;          // -1 등으로 미설정 상태 표시

        // GUI 표시용 데이터 초기화
        if (this.instructionDisplayListForGUI != null) {
            this.instructionDisplayListForGUI.clear();
        } else {
            this.instructionDisplayListForGUI = new ArrayList<>();
        }
        this.disassembledMnemonicsForGUI = null; // 로드 시 전체 니모닉 표시는 현재 사용 안함
        this.machineCodeViewForGUI = null;       // instructionDisplayListForGUI로 대체됨

        // 런타임 로그 버퍼 초기화
        this.runtimeLogBuffer.setLength(0);
    }

    /**
     * deviceManager에 등록된 모든 장치 파일 스트림을 닫습니다.
     * 프로그램 종료 또는 새 파일 로드 전에 호출될 수 있습니다.
     */
    public void closeDevice() {
        if (deviceManager == null) return;
        for (Map.Entry<String, Object> entry : deviceManager.entrySet()) {
            Object stream = entry.getValue();
            try {
                if (stream instanceof Closeable) {
                    ((Closeable) stream).close();
                }
            } catch (IOException e) {
                System.err.println("Error closing device " + entry.getKey() + ": " + e.getMessage());
            }
        }
        deviceManager.clear(); // 장치 목록 비우기
    }

    // --- 장치 입출력 메서드 (TD, RD, WD 명령어 관련) ---
    /**
     * 장치 테스트 (TD 명령어).
     * 실제 파일 시스템의 파일 존재 여부 등을 확인할 수 있습니다.
     * @param devName 테스트할 장치 이름 (파일 이름으로 간주)
     */
    public void testDevice(String devName) {
        try {
            File file = new File(devName);

            // 파일이 존재하고 읽기 가능한지 먼저 확인
            if (deviceManager.containsKey(devName) || (file.exists() && file.canRead())) {

                // 읽기/쓰기 모두 가능한 RandomAccessFile 생성
                RandomAccessFile raf = new RandomAccessFile(file, "rw");

                // 장치 매니저에 등록 (없을 때만)
                if (!deviceManager.containsKey(devName)) {
                    deviceManager.put(devName, raf);
                }

                // SW 레지스터 설정: 성공
                setRegister(REG_SW, 1);
                this.usingDevice = devName;

            } else {
                // SW 레지스터 설정: 실패
                setRegister(REG_SW, 0);
            }

        } catch (IOException e) {
            // 예외 발생 시 SW 레지스터 0으로 설정
            setRegister(REG_SW, 0);
            throw new RuntimeException("장치 열기 실패: " + devName, e);
        }
    }


    /**
     * 장치로부터 읽기 (RD 명령어).
     * @param devName 읽을 장치 이름
     * @param num 읽을 바이트 수 (SIC/XE에서는 보통 1바이트)
     * @return 읽은 데이터 (char 배열 또는 byte 배열)
     */
    public byte[] readDevice(String devName, int num) {
        RandomAccessFile raf = (RandomAccessFile) deviceManager.get(devName);
        byte[] buffer = new byte[num];

        if (raf == null) {
            System.err.println("장치 없음: " + devName);
            Arrays.fill(buffer, (byte) 0);
            return buffer;
        }

        try {
            int bytesRead = raf.read(buffer); // 최대 num 바이트 읽기

            if (bytesRead == -1) {
                // EOF
                Arrays.fill(buffer, (byte) 0);
            } else if (bytesRead < num) {
                // 일부만 읽힘: 나머지 0으로 채움
                for (int i = bytesRead; i < num; i++) {
                    buffer[i] = 0;
                }
            }

        } catch (IOException e) {
            System.err.println("읽기 오류: " + e.getMessage());
            Arrays.fill(buffer, (byte) 0);
        }

        return buffer;
    }



    /**
     * 장치로 쓰기 (WD 명령어).
     * @param devName 쓸 장치 이름
     * @param data 쓸 데이터 (char 배열 또는 byte 배열)
     * @param num 쓸 바이트 수 (SIC/XE에서는 보통 A 레지스터의 하위 1바이트)
     */
    public void writeDevice(String devName, byte[] data, int num) {
        RandomAccessFile raf = (RandomAccessFile) deviceManager.get(devName);
        if (raf == null) {
            System.err.println("장치 없음: " + devName);
            return;
        }
        try {
            raf.write(data, 0, num);  // num 바이트만 쓰기
        } catch (IOException e) {
            System.err.println("쓰기 오류 (" + devName + "): " + e.getMessage());
        }
    }


    // --- 메모리 접근 메서드 ---
    public byte[] getMemory(int location, int num) {
        if (location < 0 || location + num > memory.length || num < 0) {
            // 주소 범위를 벗어나거나 요청 길이가 음수일 경우 예외 처리 또는 빈 배열 반환
            System.err.println("Memory read out of bounds: loc=" + location + ", num=" + num);
            return new byte[Math.max(0, num)]; // 요청한 길이만큼의 빈 배열 또는 예외
        }
        byte[] buffer = new byte[num];
        System.arraycopy(memory, location, buffer, 0, num);
        return buffer;
    }

    public void setMemory(int location, byte[] data, int num) { // char[] data는 byte[] data가 더 적절할 수 있음
        if (location < 0 || location + num > memory.length || num < 0) {
            System.err.println("Memory write out of bounds: loc=" + location + ", num=" + num);
            return;
        }
        for (int i = 0; i < num; i++) {
            if (location + i < memory.length) {
                memory[location + i] = (byte) (data[i] & 0xFF); // char를 byte로 변환
            }
        }
    }

    public void writeMemory(int address, int value) { // 1바이트 쓰기 (주로 로더가 사용)
        if (address >= 0 && address < memory.length) {
            memory[address] = (byte) (value & 0xFF);
        } else {
            System.err.println("Memory write (single byte) out of bounds: addr=" + address);
        }
    }

    // --- 레지스터 접근 메서드 ---
    public int getRegister(int regNum) {
        if (regNum >= 0 && regNum < register.length) {
            return register[regNum];
        }
        System.err.println("Invalid register number for get: " + regNum);
        return 0; // 오류 시 기본값
    }

    public void setRegister(int regNum, int value) {
        if (regNum >= 0 && regNum < register.length) {
            // SIC 레지스터는 24비트. int는 32비트이므로 상위 비트가 문제될 수 있음.
            // 명령어 처리 시 24비트 범위로 값을 마스킹하는 것이 안전.
            register[regNum] = value;
        } else {
            System.err.println("Invalid register number for set: " + regNum);
        }
    }

    public double getFRegister() {
        return register_F;
    }

    public void setFRegister(double value) {
        this.register_F = value;
    }

    // --- 데이터 변환 유틸리티 ---
    public int byteToInt(byte[] data) { // 빅 엔디안 방식
        if (data == null) return 0;
        int result = 0;
        for (int i = 0; i < data.length; i++) {
            result = (result << 8) | (data[i] & 0xFF);
        }
        // SIC/XE의 24비트(3바이트) 음수 표현을 고려한다면, 추가적인 부호 확장 로직 필요
        if (data.length == 3 && (data[0] & 0x80) != 0) { // 3바이트이고 최상위 비트가 1이면 음수
            result |= 0xFF000000; // int로 부호 확장
        }
        return result;
    }

    // --- 프로그램 정보 설정/조회 ---
    public void setProgramName(String programName) { this.programName = programName; }
    public String getProgramName() { return this.programName; }

    public void setProgramStartAddress(int startAddress) { this.programStartAddr = startAddress; }
    public int getProgramStartAddress() { return this.programStartAddr; }

    public void setProgramLength(int programLength) { this.programLength = programLength; }
    public int getProgramLength() { return this.programLength; }

    public void setExeAddr(int exeAddr) { this.exeAddr = exeAddr; }
    public int getExeAddr() { return this.exeAddr; }

    // --- SymbolTable 접근 ---
    public SymbolTable getSymbolTable() { return this.symbolTable; }

    // --- GUI용 데이터 접근 ---
    public void setInstructionDisplayList(ArrayList<SicLoader.InstructionDisplayData> list) {
        this.instructionDisplayListForGUI = list;
    }


    // --- 런타임 로그 관리 ---
    public void addRuntimeLog(String message) {
        this.runtimeLogBuffer.append(message).append("\n");
    }
    public String getAndClearRuntimeLogs() {
        String logs = this.runtimeLogBuffer.toString();
        this.runtimeLogBuffer.setLength(0); // 버퍼 비우기
        return logs;
    }

    public int getUsingDevice() {
        return Integer.parseInt(usingDevice,16);
    }
}