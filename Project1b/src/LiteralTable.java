import java.util.ArrayList;


public class LiteralTable {

    ArrayList<String> literalList;
    ArrayList<Integer> locationList;
    ArrayList<String> valueList;    //objcode로 변환된 값을 저장해놓음

    public LiteralTable() {
        literalList = new ArrayList<>();
        locationList = new ArrayList<>();
        valueList = new ArrayList<>();
    }

    public void putLiteral(String literal) {
        if (!literalList.contains(literal)) {
            literalList.add(literal);
            locationList.add(-1);  // 주소는 LTORG 또는 프로그램 끝에서 할당됨
            valueList.add(null);
        }
    }

    // 필요 메서드 추가 구현
    // 리터럴 주소 할당 및 valueList 생성
    public int assignAddresses(int startLoc) {
        int addr = startLoc;

        for (int i = 0; i < literalList.size(); i++) {
            if (locationList.get(i) == -1) {
                String lit = literalList.get(i);
                String hexValue = "";
                int length = 0;

                if (lit.startsWith("=C'") && lit.endsWith("'")) {
                    String str = lit.substring(3, lit.length() - 1);
                    StringBuilder hex = new StringBuilder();
                    for (char c : str.toCharArray()) {
                        hex.append(String.format("%02X", (int) c));
                    }
                    hexValue = hex.toString();
                    length = str.length();
                }
                else if (lit.startsWith("=X'") && lit.endsWith("'")) {
                    hexValue = lit.substring(3, lit.length() - 1);
                    length = hexValue.length() / 2;
                }
                else if (lit.matches("=\\d+")) {
                    // 숫자 리터럴 처리 (=5 -> 000005)
                    int val = Integer.parseInt(lit.substring(1));
                    hexValue = String.format("%06X", val);
                    length = 3;
                }

                locationList.set(i, addr);
                valueList.set(i, hexValue);
                addr += length;
            }
        }
        return addr;
    }

    // 리터럴 주소 리턴
    public int getLocation(String literal) {
        int idx = literalList.indexOf(literal);
        return idx != -1 ? locationList.get(idx) : -1;
    }

}
