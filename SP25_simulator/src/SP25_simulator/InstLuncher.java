package SP25_simulator;

// instruction에 따라 동작을 수행하는 메소드를 정의하는 클래스

public class InstLuncher {
    ResourceManager rMgr;

    public InstLuncher(ResourceManager resourceManager) {
        this.rMgr = resourceManager;
    }

    // instruction 별로 동작을 수행하는 메소드를 정의

    public void rsub() {
        int returnAddr = rMgr.getRegister(ResourceManager.REG_L); // 복귀 주소
        rMgr.setRegister(ResourceManager.REG_PC, returnAddr);     // PC ← L
    }

    public void lda(int targetAddress, boolean isOperandImmediate){
        if(!isOperandImmediate) {
            byte[] bytes = rMgr.getMemory(targetAddress, 3);
            int value = rMgr.byteToInt(bytes);
            rMgr.setRegister(ResourceManager.REG_A, value);
        }else{
            rMgr.setRegister(ResourceManager.REG_A, targetAddress);
        }
    }

    public void stl(int targetAddress){
        int reg_L = rMgr.getRegister(rMgr.REG_L);

        //L 레지스터값 3byte로 나누기
        byte[] bytes = new byte[3];
        bytes[0] = (byte) ((reg_L>>16) & 0xFF);
        bytes[1] = (byte) ((reg_L>>8) & 0xFF);
        bytes[2] = (byte) (reg_L & 0xFF);

        rMgr.setMemory(targetAddress, bytes, 3);
    }
    public void jsub(int targetAddress){
        //L레지스터에 pc값 저장
        rMgr.setRegister(rMgr.REG_L ,rMgr.getRegister(rMgr.REG_PC));
        rMgr.setRegister(rMgr.REG_PC , targetAddress);
        return;
    }
    public void clear(int r1r2) {
        int r1 = (r1r2>>4) & 0x0F;
        int r2 = r1r2 & 0x0F;

        if(r1==ResourceManager.REG_A){rMgr.setRegister(ResourceManager.REG_A, 0);}
        if(r1==ResourceManager.REG_X){rMgr.setRegister(ResourceManager.REG_X, 0);}
        if(r1==ResourceManager.REG_L){rMgr.setRegister(ResourceManager.REG_L, 0);}
        if(r1==ResourceManager.REG_B){rMgr.setRegister(ResourceManager.REG_B, 0);}
        if(r1==ResourceManager.REG_S){rMgr.setRegister(ResourceManager.REG_S, 0);}
        if(r1==ResourceManager.REG_T){rMgr.setRegister(ResourceManager.REG_T, 0);}
        if(r1==ResourceManager.REG_F){rMgr.setFRegister(0.0);}
        if(r1==ResourceManager.REG_PC){rMgr.setRegister(ResourceManager.REG_PC, 0);}
        if(r1==ResourceManager.REG_SW){rMgr.setRegister(ResourceManager.REG_SW, 0);}
    }

    public void ldt(int targetAddrss) {
        byte[] bytes = rMgr.getMemory(targetAddrss, 3);
        int newReg = rMgr.byteToInt(bytes);
        rMgr.setRegister(ResourceManager.REG_T, newReg);
    }

    public void td(int targetAddress) {
        byte[] bytes = rMgr.getMemory(targetAddress, 1);
        int deviceCode = bytes[0] & 0xFF;  //unsigned임 → int (0~255)
        String devName = String.format("%02X", deviceCode); // "F1"
        rMgr.testDevice(devName);
    }

    public void jeq(int targetAddress){
        if (rMgr.getRegister(ResourceManager.REG_SW) == 0){
            rMgr.setRegister(ResourceManager.REG_PC, targetAddress);
        }
    }

    public void rd(int targetAddress) {
        //디바이스 이름 불러오기
        byte[] bytes = rMgr.getMemory(targetAddress, 1);
        int deviceCode = bytes[0] & 0xFF;  //unsigned임 → int (0~255)
        String devName = String.format("%02X", deviceCode); // "F1"
        //1바이트 읽어오기
        byte[] bytes1= rMgr.readDevice(devName,1);
        //얘를 A레지스터에 저장
        rMgr.setRegister(ResourceManager.REG_A,bytes1[0]);
    }

    public void compr(int r1r2){
        int r1 = rMgr.getRegister((r1r2>>4) & 0x0f);
        int r2 = rMgr.getRegister(r1r2 & 0x0f);
        if(r1==r2){
            rMgr.setRegister(ResourceManager.REG_SW, 0);
        }
        else if (r1 < r2){
            rMgr.setRegister(ResourceManager.REG_SW, -1);
        }
        else if (r1 > r2){
            rMgr.setRegister(ResourceManager.REG_SW, 1);
        }
    }

    public void stch(int targetAddress) {
        int a = rMgr.getRegister(ResourceManager.REG_A);     // int (24비트 값)
        byte lowByte = (byte) (a & 0xFF);                    // 하위 1바이트 추출
        rMgr.setMemory(targetAddress, new byte[]{lowByte}, 1); // 1바이트 저장
    }

    public void tixr(int register){
        int reg = (register >> 4) & 0xf;
        int regVal = rMgr.getRegister(reg);
        int x = rMgr.getRegister(ResourceManager.REG_X);
        x += 1;
        if(x == regVal){
            rMgr.setRegister(ResourceManager.REG_SW, 0);
        }
        else if (x < regVal) {
            rMgr.setRegister(ResourceManager.REG_SW, -1);
        }
        else if (x > regVal) {
            rMgr.setRegister(ResourceManager.REG_SW, 1);
        }
        rMgr.setRegister(ResourceManager.REG_X, x);
    }

    public void jlt(int targetAddress) {
        int sw = rMgr.getRegister(ResourceManager.REG_SW);
        if(sw<0){
            rMgr.setRegister(ResourceManager.REG_PC, targetAddress);
        }
    }

    public void stx(int targetAddress) {
        int regx = rMgr.getRegister(ResourceManager.REG_X); // 24비트 값
        byte[] bytes = new byte[3];

        bytes[0] = (byte) ((regx >> 16) & 0xFF); // 상위 바이트
        bytes[1] = (byte) ((regx >> 8) & 0xFF);  // 중간 바이트
        bytes[2] = (byte) (regx & 0xFF);         // 하위 바이트

        rMgr.setMemory(targetAddress, bytes, 3); // 3바이트 저장
    }

    public void comp(int targetAddress, boolean isOperandImmediate) {
        int val = 0;
        int a = rMgr.getRegister(ResourceManager.REG_A);
        if(isOperandImmediate){ //immediate면 입력값 그대로 사용
            val = targetAddress;
        }else{
            val = rMgr.byteToInt(rMgr.getMemory(targetAddress, 3));
        }
        if (a == val) rMgr.setRegister(ResourceManager.REG_SW, 0);
        else if (a < val) rMgr.setRegister(ResourceManager.REG_SW, -1);
        else rMgr.setRegister(ResourceManager.REG_SW, 1);
    }

    public void ldch(int targetAddress) {
        // 1. 메모리에서 1바이트 읽기
        byte[] data = rMgr.getMemory(targetAddress, 1);
        byte value = data[0]; // 읽은 바이트

        int a = rMgr.getRegister(ResourceManager.REG_A);

        a = (a & 0xFFFF00) | (value & 0xFF); // 하위 1바이트만 바꿔치기

        rMgr.setRegister(ResourceManager.REG_A, a);
    }

    public void wd(int targetAddress) {
        byte[] bytes = rMgr.getMemory(targetAddress, 1);
        int deviceCode = bytes[0] & 0xFF;  // unsigned byte → int (0~255)
        String devName = String.format("%02X", deviceCode); // 예: "F1"

        // A 레지스터에서 하위 1바이트 가져오기
        int a = rMgr.getRegister(ResourceManager.REG_A);
        byte outByte = (byte) (a & 0xFF);  // 하위 바이트만 추출

        rMgr.writeDevice(devName, new byte[]{outByte}, 1);
    }

    public void j(int targetAddress) {
        rMgr.setRegister(ResourceManager.REG_PC, targetAddress);
    }

    public void sta(int targetAddress) {
        int a = rMgr.getRegister(ResourceManager.REG_A);

        // 상위 3바이트를 추출
        byte[] value = new byte[3];
        value[0] = (byte) ((a >> 16) & 0xFF); // 상위 바이트
        value[1] = (byte) ((a >> 8) & 0xFF);  // 중간 바이트
        value[2] = (byte) (a & 0xFF);         // 하위 바이트

        rMgr.setMemory(targetAddress, value, 3); // 정확히 3바이트 저장
    }

}