# 테트리스 2.0

Visual Studio(C++20)로 만든 콘솔 테트리스입니다.
기본 테트리스의 블록 배치와 줄 제거 방식을 유지하면서, 특수 블록과 여러 게임 모드를 추가해 기존 테트리스와 차별화했습니다.
터미널 렌더링에는 [Rsact](https://github.com/MovingJu/Rsact) 라이브러리(MIT 라이선스)를 사용했습니다.

## 프로젝트 목표

- 기존 테트리스의 핵심 재미(블록 배치, 줄 제거)는 유지
- 특수 블록을 활용한 추가 전략
- 게임 모드에 따른 규칙 변화
- 같은 블록을 서로 다른 방식으로 활용하는 플레이

## 기능

### 기본 기능 (모든 모드 공통)

- 10 x 20 판, 블록 7종, 7-bag 방식 무작위 (7종을 섞어서 하나씩 꺼냄)
- 이동, 회전(간단한 벽 차기), 소프트 드롭, 하드 드롭
- 줄 제거 점수(1줄 100 / 2줄 300 / 3줄 500 / 4줄 800, 레벨 곱), 10줄마다 레벨 상승
- 다음 블록 미리보기, 고스트 블록(낙하 위치 표시)
- 홀드: 블록당 1번, 같은 종류면 교체하지 않음
- 되돌리기: 한 판에 3번, 마지막으로 놓은 블록을 취소

### 게임 모드

모드끼리는 옵션이 섞이지 않는 별도 모드입니다.

| 모드 | 옵션 | 내용 |
|---|---|---|
| STANDARD | ROTATION | OFF면 무회전 모드. 블록이 랜덤 방향으로 생성됩니다 (세로로 선 I 등) |
| STANDARD | BLOCK TYPES (1~7) | 시작할 때 N종류만 무작위로 뽑아서 그 블록만 나옵니다 |
| STANDARD | HARD BLOCK | N개를 놓을 때마다 4칸이 모두 단단한 블록이 나옵니다. 줄을 완성해도 금만 가고, 다음 줄 제거 때 함께 사라집니다 |
| BOMB | DIFFICULTY | 돌 줄 난이도 EASY / NORMAL / HARD |
| BOMB | CHAIN | ON이면 폭발에 휩쓸린 폭탄도 연쇄로 터집니다 |
| INVISIBLE | FADE TIME | 놓은 블록이 EASY 6초 / NORMAL 3초 / HARD 1.5초 뒤에 투명해집니다 |

BOMB 모드 규칙

- 블록의 한 칸이 폭탄(화면에 `*`로 표시)이며, 폭탄이 든 줄이 완성되면 그 위, 아래 줄도 함께 제거됩니다.
- 폭탄 블록은 게이지가 찰 때마다 나옵니다. (블록 1개당 +1, 줄 제거 +3, 12가 되면 폭탄 블록)
- 돌 줄(화면에 `#`)은 폭발로만 제거됩니다. NORMAL은 시작할 때 2줄, HARD는 12블록마다 바닥에서 한 줄씩 올라옵니다.

## 조작키

| 키 | 동작 |
|---|---|
| 방향키 좌/우 (A, D) | 이동 |
| 방향키 위 (W) | 회전 |
| 방향키 아래 (S) | 소프트 드롭 |
| Space | 하드 드롭 |
| C | 홀드 |
| Z (U) | 되돌리기 |
| P | 일시정지 |
| R | 재시작 |
| M | 메뉴로 돌아가기 |
| Q | 종료 |

메뉴에서는 방향키로 항목을 고르고 값을 바꾼 뒤 Enter 또는 Space로 시작합니다.

## 빌드 방법 (Windows)

필요한 것: Visual Studio 2022 (C++를 사용한 데스크톱 개발, CMake 도구), Git, Rust 1.85 이상

1. 저장소를 받습니다: `git clone https://github.com/DanRyu3/Tetris.git`
2. Visual Studio에서 파일 > 열기 > 폴더로 받은 폴더를 엽니다.
3. CMake 구성이 끝나면(처음에는 Rust 라이브러리를 빌드해서 몇 분 걸립니다) 시작 항목을 `tetris.exe`로 선택합니다.
4. 빌드한 뒤 Windows Terminal에서 실행하세요. Visual Studio 기본 디버그 콘솔에서는 실행되지 않을 수 있습니다.
5. 창 크기는 가로 50칸, 세로 28줄 이상이어야 합니다.

## 폴더 구조

| 파일 | 역할 |
|---|---|
| src/main.cpp | 메인 루프. Scene 포인터 하나로 메뉴와 게임 화면을 처리 |
| src/Scene.h | 화면의 공통 부모 클래스(추상 클래스). Menu와 Game이 상속 |
| src/Menu.h, Menu.cpp | 모드 선택 화면 |
| src/Game.h, Game.cpp | 게임 화면: 규칙, 점수, 홀드, 되돌리기, 화면 출력 |
| src/Board.h, Board.cpp | 판, 충돌 판정, 줄 제거 (폭탄, 연쇄, 단단한 블록, 돌 줄) |
| src/Piece.h, Piece.cpp | 블록 모양과 회전, 좌표 구조체 Point |
| src/Screen.h, Screen.cpp | Rsact를 감싸는 래퍼 (Rsact는 이 파일에서만 사용) |
| src/Input.h, Input.cpp | 키 입력 전용 스레드 |

## 객체지향 설계

클래스 관계

- Scene (추상 클래스)
  - Menu: 모드와 옵션 선택 화면
  - Game: 게임 화면 (Board, Piece, Rules 사용)

적용한 내용

- 상속과 다형성: Menu와 Game이 추상 클래스 Scene을 상속하고, main()은 Scene 포인터 하나로 두 화면의 handle(), update(), draw()를 호출합니다. (순수 가상 함수, 가상 소멸자, override 사용)
- 연산자 오버로딩: Point의 operator+로 블록 위치와 칸 오프셋을 더해 충돌 판정과 블록 고정에 사용합니다.
- 캡슐화와 RAII: 각 클래스의 데이터는 private으로 숨기고, Screen은 생성자와 소멸자에서 터미널 상태를 진입, 복구합니다.
- Rule of Three/Five: 터미널 자원을 가진 Screen은 복사 생성자와 복사 대입 연산자를 delete로 막았습니다.
- STL: deque와 mutex(입력 큐), vector, array, chrono(타이머), thread, unique_ptr 등

## 사용한 외부 라이브러리

- [Rsact](https://github.com/MovingJu/Rsact) (MIT License): 터미널 셀 버퍼 렌더링과 키 입력

## 제작

[류현채] ([DanRyu3](https://github.com/DanRyu3))
