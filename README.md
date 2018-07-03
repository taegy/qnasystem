# AI
A weather QnA system accepts weather-related questions and answers the questions.<br>
The source code file is located in AI folder with some libraries.
<br>
Some possible sentences to the "input:" are as follows.
<br>
오늘 날씨 어때?
<br>
내일 비가 오니?
<br>
서울과 부산 중 어디가 더워?
<br>
오늘 어제보다 추워?
<br><
Four types of questions are available.<br>
Type1. Overall weather question : The words that mean the date and the weather are needed, and the base area is Seoul.<br>
Type2. Weather factor question : It is about the specific weather element and the weather factor available is as follows; 비, 눈, 습도, 온도, 바람, 일교차, 최고온도, 최저온도. Also, several words that have similar meaning can be recognized and the collection of words exists in db.txt.<br>
Type3. Regional comparison question : It is a specific weather factor comparison between the two regions. Possible areas are as follows; 강원도, 경기도, 경상남도, 경상북도, 광주, 대구, 대전, 부산, 서울, 세종, 울산, 인천, 전라남도, 전라북도, 제주, 충청남도, 충청북도<br>
Type4. Date Comparison Question : It is a specific weather factor comparison between two dates. Possible dates are as follows; 어제, 오늘, 내일, 모레<br>
Unfortunately, only Korean language is available now and Korea Meteorological Office(kma.go.kr) provides all weather information used in this project.
