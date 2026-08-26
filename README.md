# PumpDX Rebuild

2019년에 제작한 DirectX 기반 개인 프로젝트를, 유지보수 가능한 구조와 시간 기반 리듬 게임 시스템으로 다시 만드는 Pump It Up 스타일 5패널 리듬 게임입니다.

> 현재 상태: **0단계 완료 · 1단계(런타임 골격과 데이터 경계) 진행 중 · State 05 완료**

소스는 `src/framework`(창·렌더·입력 등 재사용 가능한 기술 기반)과 `src/game`(PumpDX 전용 채보·게임플레이·씬)으로 명확히 나눕니다. 게임 코드는 framework를 사용하지만 framework는 게임 규칙을 알지 않습니다.

## 목표

- 음원 시간을 기준으로 한 정확한 판정과 스크롤 속도 독립성
- 롱노트 헤드·틱·끝 및 가변 콤보 정책
- 곡별 BGA 영상과 각 Scene의 UI 애니메이션
- 1280×720 디자인 좌표계와 해상도 독립 출력
- USB 발판 P1/P2 배정 및 2인 플레이 구조
- 브라우저 기반 채보·테마 편집기

자세한 결정은 [한글 핵심 설계](docs/DESIGN.ko.md), [구현 단계 계획](docs/IMPLEMENTATION_PLAN.ko.md)에서 확인할 수 있습니다. 영문판도 같은 범위로 함께 관리합니다.

## 현재 빌드 기반

- Windows 10/11
- C++20
- Direct3D 11
- CMake 3.24 이상
- Visual Studio의 **Desktop development with C++** 및 Windows SDK

예상 빌드 명령:

`x64 Native Tools Command Prompt for Visual Studio`에서 실행합니다.

```powershell
cmake -S . -B build --fresh -G Ninja
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Visual Studio 설치 위치가 일반 경로가 아닌 경우에도, 해당 설치의 `VsDevCmd.bat`을 먼저 실행한 개발자 명령 프롬프트에서 같은 명령을 사용합니다. Ninja 생성기는 Visual Studio의 C++ 개발 환경을 직접 사용하므로 설치 위치와 Visual Studio IDE 생성기 인식 차이에 영향을 덜 받습니다.

## 리소스와 권리

이 저장소는 비공식 팬 프로젝트이자 개인 리마스터 프로젝트입니다. Pump It Up은 Andamiro의 상표 및 게임 자산과 관련될 수 있습니다. 원작의 음악, BGA 영상, 이미지 및 기타 배포 권한이 불명확한 리소스는 이 공개 저장소에 포함하지 않습니다.
