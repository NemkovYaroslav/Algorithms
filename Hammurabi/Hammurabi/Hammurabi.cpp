#include <iostream>
#include <time.h>
#include <conio.h>
#include <random>
#include <chrono>
#include <fstream>
#include <Windows.h>
#include <string>

//Стартовые параметры игры
constexpr int kRounds = 10;       //игра состоит из 10 раундов
constexpr int kPopulation = 100;  //стартовое количество людей
constexpr double kWheat = 2800.0; //стартовое количество пшена
constexpr int kAcre = 1000;	      //стартовое количество акров
//Путь к файлу сохранения
std::string path = "saveFile.txt";

//Основополагающие параметры каждого раунда
struct Round {
	int currentRound;             //текущее значение раунда
	int currentPopulation;	      //текущее количество людей
	double currentWheat;          //текущее количество пшена
	int currentAcre;	          //текущее количество акров
	int currentStarvationVictims; //текущее количество жертв голода
	int currentAcreSow;           //количество акров под посев
	double starvPerDeaths;        //элемент статистики (среднегодовой процент умерших от голода)
};

//Расчёт текущей стоимости одного акра земли
int getAcreCost() { return 17 + rand() % (26 - 17 + 1); } // [17;26]
//Расчёт текущего количества пшеницы с акра земли
int getAcreWheat() { return 1 + rand() % (6 - 1 + 1); } // [1;6]
//Расчёт пшена, съеденного крысами
int getRatEatenWheat(double wheat) { return 0 + rand() % (int)((0.07 * wheat) - 0 + 1); } // [0;0.07*cW]
//Расчёт шанса возникновения вспышки чумы
bool getPlague() { return rand() < 0.15 * (RAND_MAX + 1.0); } // шанс 15%
//Расчёт количества жителей, приехавших в город
int getNewResidents(int starvationVictims, int acreHarvest, double wheat) {
	if (floor((double)(starvationVictims / 2) + (5 - acreHarvest) * (double)(wheat / 600) + 1) > 50) { return 50; }
	if (floor((double)(starvationVictims / 2) + (5 - acreHarvest) * (double)(wheat / 600) + 1) < 0) { return 0; }
	else { return floor((double)(starvationVictims / 2) + (5 - acreHarvest) * (double)(wheat / 600) + 1); }
}
//Проверка корректности введенных значений
int inputInt(int min = INT_MIN, int max = INT_MAX) {
	int number;
	for (;;) {
		if ((std::cin >> number) && (std::cin.good()) && (std::cin.peek() == '\n') && (number >= min) && (number <= max)) {
			return number;
		}
		if (std::cin.fail()) {
			std::cin.clear();
			std::cout << "   Мы не понимаем вас, Милорд! Повторите: ";
		}
		else {
			std::cout << "   Столько у нас нет, Милорд! Повторите: ";
		}
		std::cin.ignore(100, '\n');
	}
}
//Проверка перед кормлением жителей
int wannaEat(Round game) {
	int number;
	std::cout << "   Хочу использовать в качестве еды бушелей пшеницы: ";
	number = inputInt(0, game.currentWheat);
	if (number > game.currentPopulation * 20) {
		std::cout << "   Милорд, у вас всего " << game.currentPopulation << " жителей!" << std::endl;
		wannaEat(game);
	}
	return number;
}
//Проверка перед посевом пшена
int wannaSow(Round game) {
	int number;
	std::cout << "   Хочу засеять пшеницей акров земли: ";
	number = inputInt(0, game.currentAcre);
	if ((number * 0.5 > game.currentWheat) || (number > game.currentPopulation * 10)) {
		std::cout << "   Милорд, вам не хватает пшеницы или жителей для организации посева!" << std::endl;
		wannaSow(game);
	}
	return number;
}
//Сохранение игрового процесса
bool saveGame(Round game) {
	std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
	std::cout << " Милорд, желаете ли вы немного отдохнуть? Не переживайте, мы будет ждать вашего возвращения" << std::endl;
	std::cout << "  Приказ #1: Есть время отдохнуть. Запишите показатели (завершение игры)" << std::endl;
	std::cout << "  Приказ #2: Нет времени отдыхать. Продолжать работу! (продолжение игры)" << std::endl;
	for (;;) {
		std::cout << "   Введите # приказа: ";
		int command = inputInt(1,2);
		if (command == 1) {
			std::fstream fs;
			fs.open(path, std::fstream::out);
			if (!fs.is_open()) {
				std::cout << " Произошло недоразумение. Необходим пересмотр летописи" << std::endl;
				return true;
			}
			else {
				std::cout << " Период вашего правления занесен в летопись" << std::endl;
				fs << game.currentRound << "\n";
				fs << game.currentPopulation << "\n";
				fs << game.currentWheat << "\n";
				fs << game.currentAcre << "\n";
				fs << game.currentStarvationVictims << "\n";
				fs << game.currentAcreSow << "\n";
				fs << game.starvPerDeaths << "\n";
			}
			fs.close();
			std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
			return true;
		}
		if (command == 2) {
			std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
			return false;
		}
	}
}

//Геймплей
void gameplay(Round game) {

	//Изменяемые параметры игры
	int currentNewResidents;      //количество людей, прибывших в город
	int currentAcreHarvest;       //текущий сбор пшеницы
	int currentAcreCost;          //текущая стоимость акра
	int currentRatEatenWheat;     //текущее количество съеденого крысами пшена
	bool isPlague;                //была ли чума?
	//Регулируемые параметры игры
	int acresWannnaBuy;           //количество акров для покупки
	int acresWannaSell;           //количество акров на продажу
	int wheatWannaEat;            //количество пшена на еду жителям

	for ( ; game.currentRound <= kRounds; game.currentRound++) {
		
		//выйти и сохранить прогресс?
		if (saveGame(game)) { return; }

		std::cout << " Мой повелитель, соизволь поведать тебе" << std::endl;
		std::cout << "  Год вашего правления: " << game.currentRound << std::endl;
		if (game.currentRound == 1) {
			std::cout << "  Текущее население города: " << game.currentPopulation << std::endl;
			std::cout << "  Сколько акров сейчас занимает город: " << game.currentAcre << std::endl;
			std::cout << "  Текущее количество бушелей пшеницы: " << game.currentWheat << std::endl;
		}
		else
		{
			if (game.currentStarvationVictims > 0) {
				std::cout << "  Количество людей, умерших от голода: " << game.currentStarvationVictims << std::endl;
				if ((game.currentStarvationVictims > game.currentPopulation * 0.45)) {
					std::cout << "   Лишь немногие жители выжили, Милорд. Нас ждут тёмные времена" << std::endl;
					return;
				}
				game.currentPopulation -= game.currentStarvationVictims;
			}
			if (game.currentAcreSow > 0) {
				currentAcreHarvest = getAcreWheat();
				game.currentWheat += currentAcreHarvest * game.currentAcreSow;
			}
			else {
				currentAcreHarvest = 0;
			}
			currentNewResidents = getNewResidents(game.currentStarvationVictims, currentAcreHarvest, game.currentWheat);
			if (currentNewResidents > 0) {
				std::cout << "  Количество людей, приехавших в город: " << currentNewResidents << std::endl;
				game.currentPopulation += currentNewResidents;
			}
			isPlague = getPlague();
			if (isPlague) { game.currentPopulation -= floor((double)(game.currentPopulation / 2)); }
			std::cout << "  Текущее население города: " << game.currentPopulation << std::endl;
			std::cout << "  Была ли чума: ";
			if (isPlague) { std::cout << "да" << std::endl; }
			else { std::cout << "нет" << std::endl; }
			std::cout << "  Сколько всего пшеницы было собрано: " << currentAcreHarvest * game.currentAcreSow << std::endl;
			std::cout << "  Сколько пшеницы было получено с одного акра: " << currentAcreHarvest << std::endl;
			currentRatEatenWheat = getRatEatenWheat(game.currentWheat);
			game.currentWheat -= currentRatEatenWheat;
			std::cout << "  Сколько пшеницы уничтожили крысы: " << currentRatEatenWheat << std::endl;
			std::cout << "  Сколько акров сейчас занимает город: " << game.currentAcre << std::endl;
		}
		currentAcreCost = getAcreCost();
		std::cout << "  Какова цена одного акра земли в этом году: " << currentAcreCost << std::endl;
		std::cout << "   Хочу купить акров земли: ";
		acresWannnaBuy = inputInt(0, (int)(game.currentWheat / currentAcreCost));
		if (acresWannnaBuy > 0) {
			game.currentWheat -= acresWannnaBuy * currentAcreCost;
			game.currentAcre += acresWannnaBuy;
			std::cout << "   Милорд, вы потратили " << acresWannnaBuy * currentAcreCost << " пшеницы. Остаток: " << game.currentWheat << " единиц." << std::endl;
		}
		std::cout << "   Хочу продать акров земли: ";
		acresWannaSell = inputInt(0, game.currentAcre);
		if (acresWannaSell > 0) {
			game.currentWheat += acresWannaSell * currentAcreCost;
			game.currentAcre -= acresWannaSell;
			std::cout << "   Милорд, вы получили " << acresWannaSell * currentAcreCost << " пшеницы. Остаток: " << game.currentWheat << " единиц." << std::endl;
		}
		wheatWannaEat = wannaEat(game); // кормим жителей
		if (wheatWannaEat > 0) { game.currentWheat -= wheatWannaEat; }
		game.currentStarvationVictims = game.currentPopulation - floor((double)(wheatWannaEat / 20));
		game.currentAcreSow = wannaSow(game); // занимаемся посевом
		if (game.currentAcreSow > 0) { game.currentWheat -= game.currentAcreSow * 0.5; }
		game.starvPerDeaths += (double)game.currentStarvationVictims / game.currentPopulation; //сбор статистики по смертности
	}
	//подсчет статистики
	game.starvPerDeaths = (double)(game.starvPerDeaths / kRounds);
	int acresPerResidents = floor((double)(game.currentAcre/ game.currentPopulation));
	std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
	std::cout << " Статистика: " << floor(game.starvPerDeaths * 100) << "% среднегодовой умерших от голода / " << acresPerResidents << " акров земли на одного жителя" << std::endl;
	std::cout << "  Ваше эпоха завершена. Считается, что вы правили - ";
	if ((game.starvPerDeaths > 0.33) && (acresPerResidents < 7)) {
		std::cout << "плохо" << std::endl;
		std::cout << "  Из-за вашей некомпетентности в управлении, народ устроил бунт, и изгнал вас из города. Теперь вы вынуждены влачить жалкое существование в изгнании" << std::endl;
	}
	else {
		if ((game.starvPerDeaths > 0.10) && (acresPerResidents < 9)) {
			std::cout << "удовлетворительно" << std::endl;
			std::cout << "  Вы правили железной рукой, подобно Нерону и Ивану Грозному. Народ вздохнул с облегчением, и никто больше не желает видеть вас правителем" << std::endl;
		}
		else {
			if ((game.starvPerDeaths > 0.3) && (acresPerResidents < 10)) {
				std::cout << "хорошо" << std::endl;
				std::cout << "  Вы справились вполне неплохо, у вас, конечно, есть недоброжелатели, но многие хотели бы увидеть вас во главе города снова" << std::endl;
			}
			else {
				std::cout << "отлично" << std::endl;
				std::cout << "  Фантастика! Карл Великий, Дизраэли и Джефферсон вместе не справились бы лучше" << std::endl;
			}
		}
	}
	std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
}

int main()
{
	setlocale(LC_ALL, "RUS");
	srand(time(NULL));
	std::cout << "--------------------------------------------------------------------------------------------" << std::endl;
	Round game;
	std::fstream fs;
	fs.open(path,std::fstream::in || std::fstream::out);
	if (!fs.is_open() || fs.peek() == EOF) {
		std::cout << " Летопись о вашем правлении пуста или была утеряна! Будет начата новая летопись " << std::endl;
		game = { 1, kPopulation, kWheat, kAcre, 0, 0, 0 };
		gameplay(game);
	}
	else {
		std::cout << " Летопись о вашем правлении была найдена. Хотите ли продолжить правление? " << std::endl;
		std::cout << "  Приказ #1: Да, хочу продолжить" << std::endl;
		std::cout << "  Приказ #2: Нет, начать заного" << std::endl;
		std::cout << "   Введите # приказа: ";
		int command = inputInt(1, 2);
		if (command == 1) {
			fs >> game.currentRound;
			fs >> game.currentPopulation;
			fs >> game.currentWheat;
			fs >> game.currentAcre;
			fs >> game.currentStarvationVictims;
			fs >> game.currentAcreSow;
			fs >> game.starvPerDeaths;
			fs.close();
			gameplay(game);
		}
		if (command == 2) {
			game = { 1, kPopulation, kWheat, kAcre, 0, 0, 0 };
			gameplay(game);
		}
	}
	return EXIT_SUCCESS;
}