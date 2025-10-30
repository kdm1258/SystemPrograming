package SP25_simulator;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.io.File;

import static SP25_simulator.ResourceManager.*;     //REG_A 등등 가져올라고염ㅇㅇ

public class VisualSimulator extends JFrame {
    ResourceManager resourceManager = new ResourceManager();
    SicLoader sicLoader = new SicLoader(resourceManager); // ResourceManager 전달
    SicSimulator sicSimulator = new SicSimulator(resourceManager); // ResourceManager 전달

    // UI 요소
    private JTextField fileField;
    private JButton openButton;

    // Header Panel 요소
    private JTextField programNameField;
    private JTextField startAddressObjectProgramField;
    private JTextField lengthOfProgramField;

    // End Record Panel 요소
    private JTextField firstInstructionAddressField;

    // Memory/Instruction Panel 요소 (우측 상단)
    private JTextField startAddressInMemoryField; // 실제 메모리 시작 주소 (옵션)
    private JTextField targetAddressField;      // 계산된 TA 표시 (옵션)
    private JTextArea instructionsArea;         // 기계어 코드 표시 영역
    private JTextField deviceField;             // 사용 중인 장치 표시 (옵션)

    // Register Panel 요소 (좌측 중간)
    private JTextField registerA_DecField, registerA_HexField;
    private JTextField registerX_DecField, registerX_HexField;
    private JTextField registerL_DecField, registerL_HexField;
    private JTextField registerPC_DecField, registerPC_HexField;
    private JTextField registerSWField;

    // Register for XE Panel 요소 (좌측 하단)
    private JTextField registerB_DecField, registerB_HexField;
    private JTextField registerS_DecField, registerS_HexField;
    private JTextField registerT_DecField, registerT_HexField;
    private JTextField registerF_DecField, registerF_HexField; // F 레지스터는 보통 16진수 변환이 다름

    // Log Panel 요소 (하단)
    private JTextArea logArea;
    // 버튼
    private JButton oneStepButton;
    private JButton allStepButton;
    private JButton exitButton;

    public VisualSimulator() {
        setTitle("SIC/XE Simulator");
        setPreferredSize(new Dimension(750, 750)); // 창 크기 조정
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(10, 10)); // 컴포넌트 간 간격

        // --- 상단: 파일 입력 Panel ---
        JPanel topPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 5, 5));
        fileField = new JTextField(35); // 파일 경로 필드 크기 조정
        fileField.setEditable(false);
        openButton = new JButton("Open");
        topPanel.add(new JLabel("FileName:"));
        topPanel.add(fileField);
        topPanel.add(openButton);
        add(topPanel, BorderLayout.NORTH);

        // --- 중앙 Panel: 좌(레지스터), 우(명령어/메모리) ---
        JPanel centerPanel = new JPanel(new BorderLayout(10, 10));

        // 좌측 Panel: Header, Register, XE Register
        JPanel leftPanel = new JPanel();
        leftPanel.setLayout(new BoxLayout(leftPanel, BoxLayout.Y_AXIS)); // 수직 정렬
        leftPanel.setBorder(new EmptyBorder(0, 10, 0, 0)); // 좌측 여백
        leftPanel.add(new HeaderPanel());
        leftPanel.add(Box.createVerticalStrut(10)); // 패널 간 수직 간격
        leftPanel.add(new RegisterPanel());
        leftPanel.add(Box.createVerticalStrut(10));
        leftPanel.add(new XeRegisterPanel());
        leftPanel.add(Box.createVerticalGlue()); // 남은 공간 채우기
        centerPanel.add(leftPanel, BorderLayout.WEST);

        // 우측 Panel: End Record, Memory/Instructions
        JPanel rightPanel = new JPanel();
        rightPanel.setLayout(new BoxLayout(rightPanel, BoxLayout.Y_AXIS)); // 수직 정렬
        rightPanel.setBorder(new EmptyBorder(0, 0, 0, 10)); // 우측 여백
        rightPanel.add(new EndRecordPanel());
        rightPanel.add(Box.createVerticalStrut(10));
        rightPanel.add(createMemoryAndInstructionPanel()); // 메서드로 분리
        centerPanel.add(rightPanel, BorderLayout.CENTER);

        add(centerPanel, BorderLayout.CENTER);

        // --- 하단: Log Panel ---
        logArea = new JTextArea(8, 70); // 로그 영역 크기 조정
        logArea.setEditable(false);
        JScrollPane logScroll = new JScrollPane(logArea);
        logScroll.setBorder(BorderFactory.createTitledBorder("Log (명령어 수행 관련):"));
        logScroll.setPreferredSize(new Dimension(getWidth(), 150)); // 선호 크기 설정
        add(logScroll, BorderLayout.SOUTH);

        pack(); // 컴포넌트 크기에 맞춰 창 크기 조정
        setLocationRelativeTo(null); // 화면 중앙에 표시
        setVisible(true);

        // 버튼 리스너 설정
        setupListeners(); // 메서드명 변경 (자바 네이밍 컨벤션)

        // 초기 버튼 상태
        oneStepButton.setEnabled(false);
        allStepButton.setEnabled(false);
    }

    // 우측 Memory/Instruction 영역 생성
    private JPanel createMemoryAndInstructionPanel() {
        JPanel mainPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5); // 컴포넌트 간 간격
        gbc.anchor = GridBagConstraints.WEST; // 서쪽 정렬
        gbc.fill = GridBagConstraints.BOTH;   // 공간 채우기
        gbc.weightx = 1.0; // 가로 크기 변경 시 가중치
        gbc.weighty = 1.0; // 세로 크기 변경 시 가중치

        //Instructions Area (기계어 코드)
        JPanel instructionsPanel = new JPanel(new BorderLayout(0,3)); // 수직 간격
        instructionsPanel.add(new JLabel("Instructions:"), BorderLayout.NORTH);
        instructionsArea = new JTextArea(15, 30); // 크기 지정
        instructionsArea.setEditable(false);
        JScrollPane instructionsScroll = new JScrollPane(instructionsArea);
        instructionsScroll.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
        instructionsPanel.add(instructionsScroll, BorderLayout.CENTER);

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridheight = 2; // 2행에 걸쳐 표시
        mainPanel.add(instructionsPanel, gbc);

        //주소 입력 필드들
        JPanel topFieldsPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbcTop = new GridBagConstraints();
        gbcTop.insets = new Insets(3,3,3,3);
        gbcTop.anchor = GridBagConstraints.WEST;
        gbcTop.fill = GridBagConstraints.HORIZONTAL;
        gbcTop.weightx = 1.0;

        gbcTop.gridx = 0; gbcTop.gridy = 0;
        topFieldsPanel.add(new JLabel("Start Address in Memory:"), gbcTop);
        gbcTop.gridx = 1;
        startAddressInMemoryField = new JTextField(10);
        startAddressInMemoryField.setEditable(false); // 로더가 설정한 값 표시용
        topFieldsPanel.add(startAddressInMemoryField, gbcTop);

        gbcTop.gridx = 0; gbcTop.gridy = 1;
        topFieldsPanel.add(new JLabel("Target Address:"), gbcTop);
        gbcTop.gridx = 1;
        targetAddressField = new JTextField(10);
        targetAddressField.setEditable(false); // 계산된 TA 표시용
        topFieldsPanel.add(targetAddressField, gbcTop);

        gbc.gridx = 1; gbc.gridy = 0;
        gbc.gridheight = 1; // 1행만 차지
        gbc.weighty = 0.1;  // 세로 공간 적게 차지
        mainPanel.add(topFieldsPanel, gbc);


        // 장치 및 실행 버튼들
        JPanel controlsPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbcCtrl = new GridBagConstraints();
        gbcCtrl.insets = new Insets(5,5,5,5);
        gbcCtrl.anchor = GridBagConstraints.EAST; // 동쪽(오른쪽) 정렬
        gbcCtrl.fill = GridBagConstraints.HORIZONTAL;
        gbcCtrl.weightx = 1.0;

        gbcCtrl.gridx = 0; gbcCtrl.gridy = 0; gbcCtrl.anchor = GridBagConstraints.WEST;
        controlsPanel.add(new JLabel("사용중인 장치:"), gbcCtrl); // "Device in use:"
        gbcCtrl.gridx = 1; gbcCtrl.anchor = GridBagConstraints.EAST;
        deviceField = new JTextField(8);
        deviceField.setEditable(false); // TD, RD, WD 명령어 결과 표시용
        controlsPanel.add(deviceField, gbcCtrl);

        oneStepButton = new JButton("실행 (1 Step)");
        allStepButton = new JButton("실행 (All)");
        exitButton = new JButton("종료");

        gbcCtrl.gridx = 1; gbcCtrl.gridy = 1;
        controlsPanel.add(oneStepButton, gbcCtrl);
        gbcCtrl.gridx = 1; gbcCtrl.gridy = 2;
        controlsPanel.add(allStepButton, gbcCtrl);
        gbcCtrl.gridx = 1; gbcCtrl.gridy = 3;
        controlsPanel.add(exitButton, gbcCtrl);

        gbcCtrl.gridx = 0; gbcCtrl.gridy = 1; gbcCtrl.gridheight = 3; // 버튼들이 차지하는 공간 확보를 위해 빈 레이블 추가 가능
        controlsPanel.add(Box.createVerticalGlue(), gbcCtrl);


        gbc.gridx = 1; gbc.gridy = 1;
        gbc.weighty = 0.9; // 남은 세로 공간 차지
        gbc.anchor = GridBagConstraints.SOUTHEAST; // 아래쪽, 오른쪽 정렬
        gbc.fill = GridBagConstraints.NONE; // 버튼 크기 유지
        mainPanel.add(controlsPanel, gbc);

        return mainPanel;
    }

    // Header Record Panel (내부 클래스)
    private class HeaderPanel extends JPanel {
        public HeaderPanel() {
            setLayout(new GridBagLayout());
            setBorder(BorderFactory.createTitledBorder("H (Header Record)"));
            GridBagConstraints gbc = new GridBagConstraints();
            gbc.insets = new Insets(3, 3, 3, 3);
            gbc.anchor = GridBagConstraints.WEST;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.weightx = 1.0;

            gbc.gridx = 0; gbc.gridy = 0; add(new JLabel("Program Name:"), gbc);
            gbc.gridx = 1; programNameField = new JTextField(10); programNameField.setEditable(false); add(programNameField, gbc);

            gbc.gridx = 0; gbc.gridy = 1; add(new JLabel("<html>Start Address of <br> Object Program:</html>"), gbc);
            gbc.gridx = 1; startAddressObjectProgramField = new JTextField(10); startAddressObjectProgramField.setEditable(false); add(startAddressObjectProgramField, gbc);

            gbc.gridx = 0; gbc.gridy = 2; add(new JLabel("Length of Program:"), gbc);
            gbc.gridx = 1; lengthOfProgramField = new JTextField(10); lengthOfProgramField.setEditable(false); add(lengthOfProgramField, gbc);
        }
    }

    // Register Panel (내부 클래스)
    private class RegisterPanel extends JPanel {
        public RegisterPanel() {
            setLayout(new GridBagLayout());
            setBorder(BorderFactory.createTitledBorder("Register"));
            GridBagConstraints gbc = new GridBagConstraints();
            gbc.insets = new Insets(2, 2, 2, 5); // top, left, bottom, right
            gbc.anchor = GridBagConstraints.WEST;

            gbc.gridx = 1; gbc.gridy = 0; gbc.weightx = 0; add(new JLabel("Dec"), gbc);
            gbc.gridx = 2; add(new JLabel("Hex"), gbc);

            // A Register
            gbc.gridx = 0; gbc.gridy = 1; add(new JLabel("A (#0)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerA_DecField = new JTextField(5); registerA_DecField.setEditable(false); add(registerA_DecField, gbc);
            gbc.gridx = 2; registerA_HexField = new JTextField(5); registerA_HexField.setEditable(false); add(registerA_HexField, gbc);

            // X Register
            gbc.gridx = 0; gbc.gridy = 2; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("X (#1)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerX_DecField = new JTextField(5); registerX_DecField.setEditable(false); add(registerX_DecField, gbc);
            gbc.gridx = 2; registerX_HexField = new JTextField(5); registerX_HexField.setEditable(false); add(registerX_HexField, gbc);

            // L Register
            gbc.gridx = 0; gbc.gridy = 3; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("L (#2)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerL_DecField = new JTextField(5); registerL_DecField.setEditable(false); add(registerL_DecField, gbc);
            gbc.gridx = 2; registerL_HexField = new JTextField(5); registerL_HexField.setEditable(false); add(registerL_HexField, gbc);

            // PC Register
            gbc.gridx = 0; gbc.gridy = 4; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("PC (#8)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerPC_DecField = new JTextField(5); registerPC_DecField.setEditable(false); add(registerPC_DecField, gbc);
            gbc.gridx = 2; registerPC_HexField = new JTextField(5); registerPC_HexField.setEditable(false); add(registerPC_HexField, gbc);

            // SW Register
            gbc.gridx = 0; gbc.gridy = 5; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("SW (#9)"), gbc);
            gbc.gridx = 1; gbc.gridwidth = 2; gbc.weightx = 1.0; gbc.fill = GridBagConstraints.HORIZONTAL; registerSWField = new JTextField(12); registerSWField.setEditable(false); add(registerSWField, gbc);
        }
    }

    // XE Register Panel (내부 클래스)
    private class XeRegisterPanel extends JPanel {
        public XeRegisterPanel() {
            setLayout(new GridBagLayout());
            setBorder(BorderFactory.createTitledBorder("Register (for XE)"));
            GridBagConstraints gbc = new GridBagConstraints();
            gbc.insets = new Insets(2, 2, 2, 5);
            gbc.anchor = GridBagConstraints.WEST;
            gbc.weighty = 0; // 요소들이 위로 붙도록

            gbc.gridx = 1; gbc.gridy = 0; gbc.weightx = 0; add(new JLabel("Dec"), gbc);
            gbc.gridx = 2; add(new JLabel("Hex"), gbc);

            // B Register
            gbc.gridx = 0; gbc.gridy = 1; add(new JLabel("B (#3)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerB_DecField = new JTextField(5); registerB_DecField.setEditable(false); add(registerB_DecField, gbc);
            gbc.gridx = 2; registerB_HexField = new JTextField(5); registerB_HexField.setEditable(false); add(registerB_HexField, gbc);

            // S Register
            gbc.gridx = 0; gbc.gridy = 2; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("S (#4)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerS_DecField = new JTextField(5); registerS_DecField.setEditable(false); add(registerS_DecField, gbc);
            gbc.gridx = 2; registerS_HexField = new JTextField(5); registerS_HexField.setEditable(false); add(registerS_HexField, gbc);

            // T Register
            gbc.gridx = 0; gbc.gridy = 3; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("T (#5)"), gbc);
            gbc.gridx = 1; gbc.weightx = 0.5; gbc.fill = GridBagConstraints.HORIZONTAL; registerT_DecField = new JTextField(5); registerT_DecField.setEditable(false); add(registerT_DecField, gbc);
            gbc.gridx = 2; registerT_HexField = new JTextField(5); registerT_HexField.setEditable(false); add(registerT_HexField, gbc);

            // F Register
            gbc.gridx = 0; gbc.gridy = 4; gbc.weightx = 0; gbc.fill = GridBagConstraints.NONE; add(new JLabel("F (#6)"), gbc);
            gbc.gridx = 1; registerF_DecField = new JTextField(5); registerF_DecField.setEditable(false); add(registerF_DecField, gbc);
            gbc.gridx = 2; registerF_HexField = new JTextField(12); registerF_HexField.setEditable(false); add(registerF_HexField, gbc); // F는 48비트(12 hex chars)

            gbc.gridx = 0; gbc.gridy = 5; gbc.weighty = 1.0; // 남은 수직 공간 채우기 (아래로 밀리지 않게)
            gbc.fill = GridBagConstraints.VERTICAL;
            add(Box.createVerticalGlue(), gbc);
        }
    }

    // End Record Panel (내부 클래스)
    private class EndRecordPanel extends JPanel {
        public EndRecordPanel() {
            setLayout(new GridBagLayout());
            setBorder(BorderFactory.createTitledBorder("E (End Record)"));
            GridBagConstraints gbc = new GridBagConstraints();
            gbc.insets = new Insets(3,3,3,3);
            gbc.anchor = GridBagConstraints.WEST;
            gbc.fill = GridBagConstraints.HORIZONTAL;
            gbc.weightx = 1.0;

            gbc.gridx = 0; gbc.gridy = 0;
            add(new JLabel("<html>Address of First Instruction <br> in Object Program:</html>"), gbc);
            gbc.gridx = 1;
            firstInstructionAddressField = new JTextField(10);
            firstInstructionAddressField.setEditable(false);
            add(firstInstructionAddressField, gbc);
        }
    }

    // 이벤트 리스너 설정
    public void setupListeners() {
        openButton.addActionListener(e -> {
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setDialogTitle("SIC/XE 프로그램 파일 선택");
            FileNameExtensionFilter filter = new FileNameExtensionFilter("Object & Text Files", "obj", "txt");
            fileChooser.setFileFilter(filter);

            int select = fileChooser.showOpenDialog(VisualSimulator.this);
            if (select == JFileChooser.APPROVE_OPTION) {
                File selectedFile = fileChooser.getSelectedFile();
                fileField.setText(selectedFile.getAbsolutePath());
                try {
                    resourceManager.initializeResource(); // 리소스 및 런타임 로그 버퍼 초기화
                    logArea.setText("");                  // GUI 로그 영역 클리어

                    this.load(selectedFile); // SicLoader.load() 및 SicSimulator.load() 호출
                    this.update();           // 로드된 정보로 GUI 전체 업데이트

                    oneStepButton.setEnabled(true);
                    allStepButton.setEnabled(true);

                } catch (Exception loadException) {
                    logArea.append(" 파일 로드 중 오류 발생: " + loadException.getMessage() + "\n");
                    loadException.printStackTrace();
                    oneStepButton.setEnabled(false);
                    allStepButton.setEnabled(false);
                }
            }
        });

        oneStepButton.addActionListener(e -> {
            try {
                int ret = this.oneStep(); // sicSimulator.oneStep() 호출 (내부에서 로그 추가)
                if(ret==1) stopProg();
            } catch (Exception ex) {
                logArea.append("GUI 오류 (1-Step): " + ex.getMessage() + "\n");
                ex.printStackTrace();
                if (resourceManager != null) { // 예외 발생 시에도 버퍼에 쌓인 로그 표시 시도
                    String pendingLogs = resourceManager.getAndClearRuntimeLogs();
                    if (pendingLogs != null && !pendingLogs.isEmpty()) {
                        logArea.append(pendingLogs);
                    }
                }
            }
        });

        allStepButton.addActionListener(e -> {
            try {
                this.allStep(); // sicSimulator.allStep() 호출 (내부에서 로그 추가)
                this.update();  // GUI 새로고침
            } catch (Exception ex) {
                logArea.append("GUI 오류 (All-Step): " + ex.getMessage() + "\n");
                ex.printStackTrace();
                if (resourceManager != null) { // 예외 발생 시에도 버퍼에 쌓인 로그 표시 시도
                    String pendingLogs = resourceManager.getAndClearRuntimeLogs();
                    if (pendingLogs != null && !pendingLogs.isEmpty()) {
                        logArea.append(pendingLogs);
                    }
                }
            }
        });

        exitButton.addActionListener(e -> {
            if (resourceManager != null) {
                resourceManager.closeDevice(); // 프로그램 종료 전 사용한 장치 파일 스트림 닫기
            }
            System.exit(0);
        });
    }

    // 프로그램 로드 (SicLoader, SicSimulator의 load 호출)
    public void load(File program) {
        try {
            sicLoader.load(program);    // 메모리 적재, instructionDisplayList 생성 등
            sicSimulator.load(program); // PC 설정 등 시뮬레이터 초기화
        } catch (Exception e) {
            // VisualSimulator 레벨에서 로드 중 예외를 한 번 더 처리하여 GUI에 표시
            logArea.append("프로그램 로드 실패: " + e.getMessage() + "\n");
            e.printStackTrace();
            oneStepButton.setEnabled(false);
            allStepButton.setEnabled(false);
            // 필요시 throw e; 를 통해 호출부로 예외를 다시 던질 수 있음
        }
    }

    // 한 스텝 실행
    public int oneStep() {
        int ret = 0;
        if (sicSimulator != null) {
            try {
                ret = sicSimulator.oneStep();
            } catch (Exception e) {
                logArea.append("명령어 실행(oneStep) 중 오류: " + e.getMessage() + "\n");
                e.printStackTrace();
            }
        } else {
            logArea.append("오류: SicSimulator가 초기화되지 않았습니다.\n");
        }
        this.update();  // GUI 새로고침 (logArea에 새 런타임 로그 추가)
        return ret;
    }

    // 모든 스텝 실행
    public void allStep() {
        if (sicSimulator != null) {
            while(true){
                int currentPC = resourceManager.getRegister(ResourceManager.REG_PC);

                // 종료 조건 : 프로그램길이 벗어남
                if (currentPC < 0 || currentPC >= resourceManager.programLength) {
                    break;
                }
                // 명령어 한 줄 실행
                int ret = oneStep();
                update();
                if(ret==1) {
                    stopProg();
                    break;
                }
            }
        } else {
            logArea.append("오류: SicSimulator가 초기화되지 않았습니다.\n");
        }
    }

    // 화면 GUI 요소 업데이트
    public void update() {
        if (resourceManager == null) {
            if (logArea != null) logArea.append("오류: ResourceManager가 null입니다.\n");
            return;
        }

        // 1. H-Record 정보
        try {
            programNameField.setText(resourceManager.programName != null ? resourceManager.programName : "");
            startAddressObjectProgramField.setText(resourceManager.programStartAddr != -1 ? String.format("%06X", resourceManager.programStartAddr) : "");
            lengthOfProgramField.setText(resourceManager.programLength != 0 ? String.format("%06X", resourceManager.programLength) : "");
        } catch (Exception e) {
            if (logArea != null) logArea.append("오류(H): " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
        }

        // 2. E-Record 정보
        try {
            firstInstructionAddressField.setText(resourceManager.exeAddr != -1 ? String.format("%06X", resourceManager.exeAddr) : "");
        } catch (Exception e) {
            if (logArea != null) logArea.append("오류(E): " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
        }

        // 3. 표준 레지스터 (SIC 기본)
        try {
            int valA = resourceManager.getRegister(REG_A);
            registerA_DecField.setText(Integer.toString(valA));
            registerA_HexField.setText(String.format("%06X", valA & 0xFFFFFF)); // 24비트

            int valX = resourceManager.getRegister(REG_X);
            registerX_DecField.setText(Integer.toString(valX));
            registerX_HexField.setText(String.format("%06X", valX & 0xFFFFFF));

            int valL = resourceManager.getRegister(REG_L);
            registerL_DecField.setText(Integer.toString(valL));
            registerL_HexField.setText(String.format("%06X", valL & 0xFFFFFF));

            int valPC = resourceManager.getRegister(REG_PC);
            registerPC_DecField.setText(Integer.toString(valPC));
            registerPC_HexField.setText(String.format("%06X", valPC & 0xFFFFFF));

            registerSWField.setText(String.format("%06X", resourceManager.getRegister(REG_SW) & 0xFFFFFF));
        } catch (Exception e) {
            if (logArea != null) logArea.append("오류(Reg): " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
        }

        // 4. XE 전용 레지스터
        try {
            int valB = resourceManager.getRegister(REG_B);
            registerB_DecField.setText(Integer.toString(valB));
            registerB_HexField.setText(String.format("%06X", valB & 0xFFFFFF));

            int valS = resourceManager.getRegister(REG_S);
            registerS_DecField.setText(Integer.toString(valS));
            registerS_HexField.setText(String.format("%06X", valS & 0xFFFFFF));

            int valT = resourceManager.getRegister(REG_T);
            registerT_DecField.setText(Integer.toString(valT));
            registerT_HexField.setText(String.format("%06X", valT & 0xFFFFFF));

            double valF_double = resourceManager.getFRegister();
            registerF_DecField.setText(String.format("%f", valF_double)); // 부동소수점
            // F 레지스터 48비트 16진수 표현 (예시: Java의 Double을 long으로 변환 후 상위 48비트)
            long valF_long_bits = Double.doubleToRawLongBits(valF_double);
            registerF_HexField.setText(String.format("%012X", valF_long_bits >>> 16)); // 예시: 상위 48비트 (Java double은 64비트)

        } catch (Exception e) {
            if (logArea != null) logArea.append("오류(XE Reg): " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
        }

        // 5. Instructions Area (기계어 코드 리스트)
        try {
            if(instructionsArea != null){
                byte [] executeBytes = sicSimulator.getLastExeInst();
                if (executeBytes != null && executeBytes.length > 0){
                    StringBuilder sb = new StringBuilder();
                    for(byte b : executeBytes){
                        sb.append(String.format("%02X", b & 0xFF));
                    }
                    instructionsArea.append(sb.toString()+"\n");
                    instructionsArea.setCaretPosition(instructionsArea.getDocument().getLength());
                }
            }
        }
        catch (Exception e) {
            if (logArea != null) logArea.append("오류(Instr Area): " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
            if (instructionsArea != null) instructionsArea.setText("기계어 코드 표시 중 오류 발생");
        }

        // 6. Log Area (런타임 로그)
        if (logArea != null && resourceManager != null) {
            String newRuntimeLogs = resourceManager.getAndClearRuntimeLogs();
            if (newRuntimeLogs != null && !newRuntimeLogs.isEmpty()) {
                logArea.append(newRuntimeLogs);
                logArea.setCaretPosition(logArea.getDocument().getLength()); // 스크롤 맨 아래로
            }
        }

        // 7. startAddressInMemoryField
        if (startAddressInMemoryField != null && resourceManager.programStartAddr != -1) {
            startAddressInMemoryField.setText(String.format("%06X", resourceManager.programStartAddr));
        }
        // 8. targetaddress Field
        try {
            if(targetAddressField != null){
                int targetAddress = sicSimulator.getTA();
                targetAddressField.setText(String.format("%06X", targetAddress));
            }
        }
        catch (Exception e) {
            if (logArea != null) logArea.append("오류: " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
            if (instructionsArea != null) instructionsArea.setText("TA 표시 중 오류 발생");
        }
        // 9. device Field
        try {
            if(deviceField != null){
                int device = 0;
                if (resourceManager != null) {
                    device = resourceManager.getUsingDevice();
                }
                deviceField.setText(String.format("%02X", device));
            }
        }
        catch (Exception e) {
            if (logArea != null) logArea.append("오류: " + e.getMessage().substring(0, Math.min(e.getMessage().length(), 50)) + "\n");
            if (instructionsArea != null) instructionsArea.setText("TA 표시 중 오류 발생");
        }

    }
    public void stopProg(){
        allStepButton.setEnabled(false);
        oneStepButton.setEnabled(false);
    }


    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new VisualSimulator());
    }
}