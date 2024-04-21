***********
# [WEEK06] 탐험준비 - 웹서버 만들기
📢 “정글끝까지 가기 전에, 준비운동을 하며 필수 스킬을 익혀봅시다!”

3주간 각 1주차 씩 Red-Black tree → malloc → 웹 proxy 서버를 C언어로 구현하면서, C언어 포인터의 개념, gdb 디버거 사용법 등을 익혀봅니다. 또한, Segmentation fault 등 새로운 에러들을 마주해봅니다! 🙂
알고리즘(CLRS), 컴퓨터 시스템(CS:APP) 교재를 참고하여 진행합니다.
RB tree - CLRS 13장, malloc - CS:APP 9장, 웹서버 - CS:APP 11장

***********
💡 Ubuntu 22.04 LTS (x86_64)환경을 사용합니다.

개발 환경 설치
```ubuntu
sudo apt update                         # package list update
sudo apt upgrade                        # upgrade packages
sudo apt install gcc make valgrind gdb  # gcc, make 등 개발 환경 설치
```

GitHub 토큰 관리를 위한 gh 설치 
```
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
sudo chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
sudo apt update
sudo apt install gh
```
*(컨테이너에 GitHub Cli를 추가적으로 설치해놓기는 함)*
*Docker 관리를 위해 in Docker를 추가해 놓아서 용량이 조금 크다*
**********************************

```

```

**********************************
```
./driver.sh #를 이용하여 점수 측정 
```
**********************************
