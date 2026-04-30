<h1 align="center">Deep-Live-Cam 2.1</h1>

<p align="center">
  단 하나의 이미지만으로 실시간 얼굴 교체 및 영상 딥페이크를 클릭 한 번에 구현합니다.
</p>

<p align="center">
<a href="https://trendshift.io/repositories/11395" target="_blank"><img src="https://trendshift.io/api/badge/repositories/11395" alt="hacksider%2FDeep-Live-Cam | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>
</p>

<p align="center">
  <img src="media/demo.gif" alt="Demo GIF" width="800">
</p>

## 면책 조항

이 딥페이크 소프트웨어는 AI 생성 미디어 산업에서 생산적인 도구로 활용되도록 설계되었습니다. 아티스트가 커스텀 캐릭터를 애니메이션화하고, 매력적인 콘텐츠를 만들거나 의류 디자인에 모델을 활용하는 데 도움을 줄 수 있습니다.

비윤리적인 사용 가능성을 인지하고 있으며, 예방 조치를 위해 노력하고 있습니다. 프로그램에는 부적절한 미디어(노출, 폭력적 콘텐츠, 전쟁 장면 등 민감한 소재)를 처리하는 것을 방지하는 내장 검사 기능이 있습니다. 법률과 윤리를 준수하며 이 프로젝트를 책임감 있게 계속 개발해 나갈 것입니다. 법적으로 요구될 경우 프로젝트를 중단하거나 워터마크를 추가할 수 있습니다.

- **윤리적 사용**: 사용자는 소프트웨어를 책임감 있고 합법적으로 사용해야 합니다. 실제 인물의 얼굴을 사용하는 경우, 해당 인물의 동의를 받고 온라인 공유 시 딥페이크임을 명확히 표시하세요.

- **콘텐츠 제한**: 소프트웨어에는 노출, 폭력적 콘텐츠 등 부적절한 미디어 처리를 방지하는 내장 검사 기능이 포함되어 있습니다.

- **법적 준수**: 모든 관련 법률과 윤리 지침을 준수합니다. 법적으로 요구될 경우 프로젝트를 중단하거나 출력물에 워터마크를 추가할 수 있습니다.

- **사용자 책임**: 최종 사용자의 행동에 대해 책임을 지지 않습니다. 사용자는 소프트웨어 사용이 윤리적 기준 및 법적 요건에 부합하는지 확인해야 합니다.

이 소프트웨어를 사용함으로써 위 조건에 동의하고, 타인의 권리와 존엄성을 존중하는 방식으로 사용할 것을 약속합니다.

## 독점 v2.7 베타 빠른 시작 - 사전 빌드 버전 (Windows/Mac Silicon/CPU)

  <a href="https://deeplivecam.net/index.php/quickstart"> <img src="media/Download.png" width="285" height="77" />

##### NVIDIA 또는 AMD 전용 GPU, CPU 또는 Mac Silicon이 있다면 이 버전이 가장 빠르며 우선 지원을 받을 수 있습니다. v2.7 베타는 오픈소스 버전보다 30가지 이상의 추가 기능을 제공합니다.

###### 이 사전 빌드 버전은 기술적 지식이 없거나 모든 요구 사항을 수동으로 설치할 시간이 없는 사용자에게 적합합니다. 오픈소스 프로젝트이므로 수동 설치도 가능합니다.

## 요약: 클릭 3번으로 라이브 딥페이크

![easysteps](https://github.com/user-attachments/assets/af825228-852c-411b-b787-ffd9aac72fc6)
1. 얼굴 선택
2. 사용할 카메라 선택
3. 라이브 버튼 누르기!

## 기능 및 활용 - 모든 것이 실시간으로

### 입 마스크 (Mouth Mask)

**입 마스크를 사용하여 정확한 움직임을 위해 원래 입을 유지하세요**

<p align="center">
  <img src="media/ludwig.gif" alt="resizable-gif">
</p>

### 얼굴 매핑 (Face Mapping)

**여러 인물에 동시에 다양한 얼굴 사용하기**

<p align="center">
  <img src="media/streamers.gif" alt="face_mapping_source">
</p>

### 나만의 영화, 나만의 얼굴

**실시간으로 어떤 얼굴로든 영화 감상하기**

<p align="center">
  <img src="media/movie.gif" alt="movie">
</p>

### 라이브 쇼

**라이브 쇼와 공연 진행하기**

<p align="center">
  <img src="media/live_show.gif" alt="show">
</p>

### 밈 (Memes)

**가장 바이럴한 밈 만들기**

<p align="center">
  <img src="media/meme.gif" alt="show" width="450">
  <br>
  <sub>Deep-Live-Cam의 여러 얼굴 기능으로 제작됨</sub>
</p>

## 설치 (수동)

**설치에는 기술적 지식이 필요하며 초보자에게는 적합하지 않습니다. 빠른 시작 버전 다운로드를 권장합니다.**

<details>
<summary>클릭하여 설치 과정 보기</summary>

### 설치

CPU를 사용하므로 속도는 느리지만 대부분의 컴퓨터에서 작동합니다.

**1. 플랫폼 설정**

-   Python (3.11 권장)
-   pip
-   git
-   [ffmpeg](https://www.youtube.com/watch?v=OlNWCpFdVMA) - ```iex (irm ffmpeg.tc.ht)```
-   [Visual Studio 2022 런타임 (Windows)](https://visualstudio.microsoft.com/visual-cpp-build-tools/)

**2. 저장소 복제**

```bash
git clone https://github.com/hacksider/Deep-Live-Cam.git
cd Deep-Live-Cam
```

**3. 모델 다운로드**

1. [GFPGANv1.4](https://huggingface.co/hacksider/deep-live-cam/resolve/main/GFPGANv1.4.onnx)
2. [inswapper\_128\_fp16.onnx](https://huggingface.co/hacksider/deep-live-cam/resolve/main/inswapper_128_fp16.onnx)

이 파일들을 "**models**" 폴더에 넣으세요.

**4. 의존성 설치**

문제를 방지하기 위해 `venv` 사용을 강력히 권장합니다.

Windows의 경우:
```bash
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```

Linux의 경우:
```bash
# 설치된 Python 3.10을 사용하세요
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

**macOS의 경우:**

Apple Silicon (M1/M2/M3)은 별도 설정이 필요합니다:

```bash
# Python 3.11 설치 (특정 버전이 중요합니다)
brew install python@3.11

# tkinter 패키지 설치 (GUI에 필요)
brew install python-tk@3.10

# Python 3.11로 가상 환경 생성 및 활성화
python3.11 -m venv venv
source venv/bin/activate

# 의존성 설치
pip install -r requirements.txt
```

**가상 환경을 재설치해야 하는 경우:**

```bash
# 가상 환경 비활성화 후 삭제
rm -rf venv

# 가상 환경 재설치
python -m venv venv
source venv/bin/activate

# 의존성 재설치
pip install -r requirements.txt

# gfpgan 및 basicsrs 오류 수정
pip install git+https://github.com/xinntao/BasicSR.git@master
pip uninstall gfpgan -y
pip install git+https://github.com/TencentARC/GFPGAN.git@master
```

**실행:** GPU가 없다면 `python run.py`로 실행할 수 있습니다. 첫 실행 시 모델을 다운로드합니다 (~300MB).

### GPU 가속

**CUDA 실행 제공자 (Nvidia)**

1. [CUDA Toolkit 12.8.0](https://developer.nvidia.com/cuda-12-8-0-download-archive) 설치
2. [cuDNN v8.9.7 for CUDA 12.x](https://developer.nvidia.com/rdp/cudnn-archive) 설치 (onnxruntime-gpu에 필요):
   - CUDA 12.x용 cuDNN v8.9.7 다운로드
   - cuDNN bin 디렉터리가 시스템 PATH에 포함되어 있는지 확인
3. 의존성 설치:

```bash
pip install -U torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu128
pip uninstall onnxruntime onnxruntime-gpu
pip install onnxruntime-gpu==1.21.0
```

4. 실행:

```bash
python run.py --execution-provider cuda
```

**CoreML 실행 제공자 (Apple Silicon)**

Apple Silicon (M1/M2/M3) 전용 설치:

1. 위의 macOS 설정을 Python 3.10으로 완료했는지 확인하세요.
2. 의존성 설치:

```bash
pip uninstall onnxruntime onnxruntime-silicon
pip install onnxruntime-silicon==1.13.1
```

3. 실행 (Python 3.10 지정이 중요합니다):

```bash
python3.10 run.py --execution-provider coreml
```

**macOS 중요 참고사항:**
- Python 3.11 이상이 아닌 **반드시 Python 3.10**을 사용해야 합니다
- 여러 Python 버전이 설치된 경우 `python` 대신 `python3.10` 명령을 사용하세요
- `_tkinter` 누락 오류가 발생하면 tkinter 패키지를 재설치하세요: `brew reinstall python-tk@3.10`
- 모델 로딩 오류가 발생하면 모델이 올바른 폴더에 있는지 확인하세요
- 다른 Python 버전과 충돌이 발생하면 해당 버전 제거를 고려하세요:
  ```bash
  # 설치된 Python 버전 목록 확인
  brew list | grep python

  # 필요한 경우 충돌하는 버전 제거
  brew uninstall --ignore-dependencies python@3.11 python@3.13

  # Python 3.11만 유지
  brew cleanup
  ```

**CoreML 실행 제공자 (Apple 레거시)**

1. 의존성 설치:

```bash
pip uninstall onnxruntime onnxruntime-coreml
pip install onnxruntime-coreml==1.21.0
```

2. 실행:

```bash
python run.py --execution-provider coreml
```

**DirectML 실행 제공자 (Windows)**

1. 의존성 설치:

```bash
pip uninstall onnxruntime onnxruntime-directml
pip install onnxruntime-directml==1.21.0
```

2. 실행:

```bash
python run.py --execution-provider directml
```

**OpenVINO™ 실행 제공자 (Intel)**

1. 의존성 설치:

```bash
pip uninstall onnxruntime onnxruntime-openvino
pip install onnxruntime-openvino==1.21.0
```

2. 실행:

```bash
python run.py --execution-provider openvino
```
</details>

## 사용 방법

**1. 이미지/영상 모드**

-   `python run.py`를 실행합니다.
-   소스 얼굴 이미지와 타겟 이미지 또는 영상을 선택합니다.
-   "Start(시작)" 버튼을 클릭합니다.
-   출력 결과는 타겟 영상 이름으로 생성된 디렉터리에 저장됩니다.

**2. 웹캠 모드**

-   `python run.py`를 실행합니다.
-   소스 얼굴 이미지를 선택합니다.
-   "Live(라이브)" 버튼을 클릭합니다.
-   미리보기가 나타날 때까지 기다립니다 (10~30초).
-   OBS 같은 화면 캡처 도구를 사용하여 스트리밍합니다.
-   얼굴을 바꾸려면 새 소스 이미지를 선택하세요.

## Hugging Face에서 모든 모델 다운로드
- [**여기서 모델 다운로드**](https://huggingface.co/hacksider/deep-live-cam/tree/main)

## 명령줄 인수 (비유지 관리)

```
옵션:
  -h, --help                                               도움말 메시지 출력 후 종료
  -s SOURCE_PATH, --source SOURCE_PATH                     소스 이미지 선택
  -t TARGET_PATH, --target TARGET_PATH                     타겟 이미지 또는 영상 선택
  -o OUTPUT_PATH, --output OUTPUT_PATH                     출력 파일 또는 디렉터리 선택
  --frame-processor FRAME_PROCESSOR [FRAME_PROCESSOR ...]  프레임 처리기 (선택: face_swapper, face_enhancer, ...)
  --keep-fps                                               원본 FPS 유지
  --keep-audio                                             원본 오디오 유지
  --keep-frames                                            임시 프레임 유지
  --many-faces                                             모든 얼굴 처리
  --map-faces                                              소스-타겟 얼굴 매핑
  --mouth-mask                                             입 영역 마스킹
  --video-encoder {libx264,libx265,libvpx-vp9}             출력 영상 인코더 조정
  --video-quality [0-51]                                   출력 영상 품질 조정
  --live-mirror                                            전면 카메라처럼 라이브 카메라 화면 미러링
  --live-resizable                                         라이브 카메라 프레임 크기 조정 가능
  --max-memory MAX_MEMORY                                  최대 RAM 사용량 (GB)
  --execution-provider {cpu} [{cpu} ...]                   사용 가능한 실행 제공자 (선택: cpu, ...)
  --execution-threads EXECUTION_THREADS                    실행 스레드 수
  -v, --version                                            프로그램 버전 표시 후 종료
```

CLI 모드로 실행하려면 `-s/--source` 인수를 사용하세요.

## 언론 보도

 - [**Ars Technica**](https://arstechnica.com/information-technology/2024/08/new-ai-tool-enables-real-time-face-swapping-on-webcams-raising-fraud-concerns/) - *"Deep-Live-Cam이 바이럴되며 누구나 디지털 도플갱어가 될 수 있게 되었다"*
 - [**Yahoo!**](https://www.yahoo.com/tech/ok-viral-ai-live-stream-080041056.html) - *"이 바이럴 AI 라이브 스트림 소프트웨어는 정말 무섭다"*
 - [**CNN Brasil**](https://www.cnnbrasil.com.br/tecnologia/ia-consegue-clonar-rostos-na-webcam-entenda-funcionamento/) - *"AI가 웹캠에서 얼굴을 복제할 수 있다; 작동 원리를 이해하다"*
 - [**PetaPixel**](https://petapixel.com/2024/08/14/deep-live-cam-deepfake-ai-tool-lets-you-become-anyone-in-a-video-call-with-single-photo-mark-zuckerberg-jd-vance-elon-musk/) - *"딥페이크 AI 도구가 사진 한 장으로 영상 통화에서 누구든 될 수 있게 해준다"*

## 크레딧

-   [ffmpeg](https://ffmpeg.org/): 영상 관련 작업을 쉽게 만들어 준 것에 감사
-   [Henry](https://github.com/henryruhs): 주요 기여자
-   [deepinsight](https://github.com/deepinsight): 잘 만들어진 라이브러리와 모델을 제공한 [insightface](https://github.com/deepinsight/insightface) 프로젝트에 감사. [모델은 비상업적 연구 목적으로만 사용 가능](https://github.com/deepinsight/insightface?tab=readme-ov-file#license)합니다.
-   [havok2-htwo](https://github.com/havok2-htwo): 웹캠 코드 공유에 감사
-   [GosuDRM](https://github.com/GosuDRM): roop의 오픈 버전에 감사
-   [pereiraroland26](https://github.com/pereiraroland26): 여러 얼굴 지원
-   [qitianai](https://github.com/qitianai): 다국어 지원
-   그리고 이 프로젝트에서 사용된 라이브러리 뒤의 [모든 개발자들](https://github.com/hacksider/Deep-Live-Cam/graphs/contributors)에게 감사합니다.
-   코드의 원저자는 [s0md3v](https://github.com/s0md3v/roop)입니다.
-   저장소에 별을 달아 이 프로젝트를 바이럴하게 만들어 준 모든 멋진 사용자들에게 감사합니다 ❤️

## 기여

![Alt](https://repobeats.axiom.co/api/embed/fec8e29c45dfdb9c5916f3a7830e1249308d20e1.svg "Repobeats analytics image")
