import java.io.*;
import java.util.HashMap;


/**
 * ��� instruction�� ������ �����ϴ� Ŭ����. instruction data���� �����Ѵ�. <br>
 * ���� instruction ���� ����, ���� ��� ����� �����ϴ� �Լ�, ���� ������ �����ϴ� �Լ� ���� ���� �Ѵ�.
 */
public class InstTable {
	/** 
	 * inst.data ������ �ҷ��� �����ϴ� ����.
	 *  ��ɾ��� �̸��� ��������� �ش��ϴ� Instruction�� �������� ������ �� �ִ�.
	 */
	HashMap<String, Instruction> instMap;
	
	/**
	 * Ŭ���� �ʱ�ȭ. �Ľ��� ���ÿ� ó���Ѵ�.
	 * @param instFile : instuction�� ���� ���� ����� ���� �̸�
	 */
	public InstTable(String instFile) {
		instMap = new HashMap<String, Instruction>();
		openFile(instFile);
	}
	
	/**
	 * �Է¹��� �̸��� ������ ���� �ش� ������ �Ľ��Ͽ� instMap�� �����Ѵ�.
	 */
	public void openFile(String fileName) {
		try(BufferedReader reader = new BufferedReader(new FileReader(fileName))){
			String line;
			while((line = reader.readLine()) != null){
				line = line.trim();
				if(line.isEmpty()) continue;	//���� skip

				Instruction inst = new Instruction(line);
				instMap.put(inst.name, inst);
			}
		}
		catch(IOException e){
			e.printStackTrace();
		}
	}

	public Instruction searchOpcode(String checkop){
		return instMap.get(checkop);
	}
	//get, set, search ���� �Լ��� ���� ����

}
/**
 * ��ɾ� �ϳ��ϳ��� ��ü���� ������ InstructionŬ������ ����.
 * instruction�� ���õ� �������� �����ϰ� �������� ������ �����Ѵ�.
 */
class Instruction {
	/* 
	 * ������ inst.data ���Ͽ� �°� �����ϴ� ������ �����Ѵ�.
	 *  
	 * ex)
	 * String instruction;
	 * int opcode;
	 * int numberOfOperand;
	 * String comment;
	 */

	String name;	//��ɾ� �̸�
	int opcode;		//��ɾ� opcode
	int operandCnt;	//operand ����
	int format; 	// instruction�� �� ����Ʈ ��ɾ����� ����. ���� ���Ǽ��� ����
	
	/**
	 * Ŭ������ �����ϸ鼭 �Ϲݹ��ڿ��� ��� ������ �°� �Ľ��Ѵ�.
	 * @param line : instruction �����Ϸκ��� ���پ� ������ ���ڿ�
	 */
	public Instruction(String line) {
		parsing(line);
	}
	
	/**
	 * �Ϲ� ���ڿ��� �Ľ��Ͽ� instruction ������ �ľ��ϰ� �����Ѵ�.
	 * @param line : instruction �����Ϸκ��� ���پ� ������ ���ڿ�
	 */
	public void parsing(String line) {
		String[] token = line.split("\t");	//������ ��ūȭ
		if(token.length < 4) return; 				//�̻��� ���ϳ��� �Ÿ���

		/**inst name ��������**/
		name = token[0].trim();

		/**format ��������**/
		String formatStr = token[1].trim();
		if(formatStr.contains("/")){
			format = Integer.parseInt(formatStr.split("/")[0]);	//"3/4"�� ��� 3�� ����
		}
		else{
			format = Integer.parseInt(formatStr);
		}

		/**opcode 16������ ��������**/
		opcode = Integer.parseInt(token[2].trim(), 16);

		/**operand ���� ��������**/
		operandCnt = Integer.parseInt(token[3].trim());
	}
	
		
	//�� �� �Լ� ���� ����
	
	
}
