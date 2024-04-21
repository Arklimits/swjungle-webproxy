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
*** Basic ***
Starting tiny on 2012
Starting proxy on 24723
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 23461
Starting proxy on port 26032
Starting the blocking NOP server on port 20321
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15

*** Cache ***
Starting tiny on port 26070
Starting proxy on port 23140
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```

**********************************
```
./driver.sh #를 이용하여 점수 측정 
```
**********************************
