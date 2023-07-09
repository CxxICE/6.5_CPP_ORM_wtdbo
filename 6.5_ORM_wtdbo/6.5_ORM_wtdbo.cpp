#include "windows.h"

#include <iostream>
#include <memory>
#include <ctime>

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <Wt/Dbo/WtSqlTraits.h>//поддержка UTF-8
//#include <Wt/WString.h>

class Publisher;
class Book;
class Shop;
class Stock;
class Sale;

//отключение поля version и создание своего поля id для всех классов
namespace Wt 
{
	namespace Dbo 
	{

		template<>
		struct dbo_traits<Publisher> : public dbo_default_traits
		{
			static const char *surrogateIdField()
			{
				return "id_publisher";
			};
			static const char *versionField() {
				return nullptr;
			};
		};
		template<>
		struct dbo_traits<Book> : public dbo_default_traits
		{
			static const char *surrogateIdField()
			{
				return "id_book";
			};
			static const char *versionField() {
				return nullptr;
			};
		};
		template<>
		struct dbo_traits<Shop> : public dbo_default_traits
		{
			static const char *surrogateIdField()
			{
				return "id_shop";
			};
			static const char *versionField() {
				return nullptr;
			};
		};
		template<>
		struct dbo_traits<Stock> : public dbo_default_traits
		{
			static const char *surrogateIdField()
			{
				return "id_stock";
			};
			static const char *versionField() {
				return nullptr;
			};
		};
		template<>
		struct dbo_traits<Sale> : public dbo_default_traits
		{
			static const char *surrogateIdField()
			{
				return "id_sale";
			};
			static const char *versionField() {
				return nullptr;
			};
		};

	}
}

class Publisher
{
public:
	Wt::WString name = L"";
	Wt::Dbo::collection<Wt::Dbo::ptr<Book>> books;

	Publisher() = default;

	Publisher(const Wt::WString &_name)
	{
		name = _name;
	};

	template<class Action>
	void persist(Action &a)
	{
		Wt::Dbo::field(a, name, "name");
		Wt::Dbo::hasMany(a, books, Wt::Dbo::ManyToOne, "id_publisher");
	};
};

class Book
{
public:
	Wt::WString title = L"";
	Wt::Dbo::ptr<Publisher> publisher;
	Wt::Dbo::collection<Wt::Dbo::ptr<Stock>> stocks;	

	Book() = default;

	Book(const Wt::WString &_title, const Wt::Dbo::ptr<Publisher> &_publisher)
	{
		title = _title;
		publisher = _publisher;
	};

	template<class Action>
	void persist(Action &a)
	{
		Wt::Dbo::field(a, title, "title");
		Wt::Dbo::belongsTo(a, publisher, ">id_publisher",//знак > в имени поля id_publisher позволяет избежать конкатенации id_field
			Wt::Dbo::NotNull |
			Wt::Dbo::OnUpdateCascade |
			Wt::Dbo::OnDeleteRestrict);
		Wt::Dbo::hasMany(a, stocks, Wt::Dbo::ManyToOne, "id_book");		
	};
};

class Shop
{
public:
	Wt::WString name = L"";
	Wt::Dbo::collection<Wt::Dbo::ptr<Stock>> stocks;

	Shop() = default;

	Shop(const Wt::WString &_name)
	{
		name = _name;
	};

	template<class Action>
	void persist(Action &a)
	{
		Wt::Dbo::field(a, name, "name");
		Wt::Dbo::hasMany(a, stocks, Wt::Dbo::ManyToOne, "id_shop");
	};
};

class Stock
{
public:
	int count = 0;
	Wt::Dbo::ptr<Book> book;
	Wt::Dbo::ptr<Shop> shop;
	Wt::Dbo::collection<Wt::Dbo::ptr<Sale>> sales;

	Stock() = default;

	Stock(const Wt::Dbo::ptr<Book> &_book, const Wt::Dbo::ptr<Shop> &_shop, int _count)
	{
		book = _book;
		shop = _shop;
		count = _count;
	};

	template<class Action>
	void persist(Action &a)
	{
		Wt::Dbo::field(a, count, "count");
		Wt::Dbo::belongsTo(a, book, ">id_book",
			Wt::Dbo::NotNull |
			Wt::Dbo::OnUpdateCascade |
			Wt::Dbo::OnDeleteRestrict);
		Wt::Dbo::belongsTo(a, shop, ">id_shop",
			Wt::Dbo::NotNull |
			Wt::Dbo::OnUpdateCascade |
			Wt::Dbo::OnDeleteRestrict);
		Wt::Dbo::hasMany(a, sales, Wt::Dbo::ManyToOne, "id_stock");
	};

};

class Sale
{
public:
	double price = 0;
	int count = 0;
	Wt::WString date_sale = "";
	Wt::Dbo::ptr<Stock> stock;

	Sale() = default;

	Sale(const Wt::Dbo::ptr<Stock> &_stock, double _price, int _count, std::string &_date_sale)
	{
		stock = _stock;
		price = _price;
		count = _count;
		date_sale = _date_sale;
	};

	template<class Action>
	void persist(Action &a)
	{
		Wt::Dbo::field(a, price, "price");
		Wt::Dbo::field(a, date_sale, "date_sale");
		Wt::Dbo::field(a, count, "count");
		Wt::Dbo::belongsTo(a, stock, ">id_stock",
			Wt::Dbo::NotNull |
			Wt::Dbo::OnUpdateCascade |
			Wt::Dbo::OnDeleteRestrict);
	};
};

void publisher_query(Wt::Dbo::Session &session, std::wstring &string)
{
	Wt::Dbo::Transaction transaction{ session };
	using Result = Wt::Dbo::ptr<Shop>;
	auto q1 = session.query<Result>("SELECT DISTINCT sh FROM shops sh").join<Stock>("st", "sh.id_shop = st.id_shop").join<Book>("b", "b.id_book = st.id_book").join<Publisher>("p", "b.id_publisher = p.id_publisher").where("p.name = ?");
	auto result = q1.bind(Wt::WString(string)).resultList();
	if (result.size() > 0)
	{
		std::wcout << L"Магазины в которых продают книги издателя \"" << string << L"\":\n";
		for (const Result &r : result)
		{
			std::wcout << r->name << L"\n";
		}
	}
	else
	{
		std::wcout << L"Ни в одном магазине нет книг издателя \"" << string << L"\".\n";
	}
	transaction.commit();
}

int main()
{
	setlocale(LC_ALL, "ru_RU.UTF8");
	SetConsoleOutputCP(65001);
	SetConsoleCP(1251);
	//std::srand(time(NULL));
	try
	{
		std::string connectionString =
			"host=localhost"
			" port=5432"
			" dbname=bookshop"
			" user=Ilya"
			" password=123zxcZXC";
		auto postgres = std::make_unique<Wt::Dbo::backend::Postgres>(connectionString);

		Wt::Dbo::Session session;
		session.setConnection(std::move(postgres));
		
		session.mapClass<Publisher>("publishers");
		session.mapClass<Book>("books");
		session.mapClass<Shop>("shops");
		session.mapClass<Stock>("stocks");
		session.mapClass<Sale>("sales");
			
		try//удаление таблиц если есть
		{
			session.dropTables();
			std::wcout << L"Таблицы удалены." << std::endl;
		}
		catch (const Wt::Dbo::Exception &err)
		{
			std::cout << err.what() << std::endl;
		}
		
		try//создание таблиц
		{
			session.createTables();
			Wt::Dbo::Transaction transaction{ session };
			session.execute("ALTER TABLE sales ALTER COLUMN price TYPE numeric(20,2);");
			session.execute("ALTER TABLE sales DROP COLUMN date_sale;");
			session.execute("ALTER TABLE sales ADD COLUMN date_sale date;");
			transaction.commit();
			std::wcout << L"Таблицы созданы." << std::endl;
		}
		catch (const Wt::Dbo::Exception &err)
		{
			std::cout << err.what() << std::endl;
		}
		
		//Заполнение таблиц
		
		Wt::Dbo::Transaction transaction{ session };
		session.addNew<Publisher>(L"ЛитРес");
		session.addNew<Publisher>(L"DeAgostini");
		session.addNew<Publisher>(L"Эксмо");
		session.addNew<Publisher>(L"Дрофа");
		session.addNew<Publisher>(L"Лабиринт");

		session.addNew<Shop>(L"Книжный на Бауманской");
		session.addNew<Shop>(L"Книжный на Тверской");
		session.addNew<Shop>(L"Книжный на Арбате");
		session.addNew<Shop>(L"Книжный у Кремля");
		session.addNew<Shop>(L"Книжный на Покровке");
		
		//Wt::Dbo::Query<Wt::Dbo::ptr<Publisher>> q1 = session.find<Publisher>().where("name = ?");

		session.addNew<Book>(L"Джек Лондон, рассказы", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"ЛитРес")));
		session.addNew<Book>(L"Марк Твен «Приключения Гекльберри Финна»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"ЛитРес")));
		session.addNew<Book>(L"Максим Горький «Макар Чудра»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"ЛитРес")));
		
		session.addNew<Book>(L"Марк Твен «История с привидением»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"DeAgostini")));
		session.addNew<Book>(L"Николай Гоголь «Тарас Бульба»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"DeAgostini")));
		session.addNew<Book>(L"Герберт Уэллс «Война миров»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"DeAgostini")));

		session.addNew<Book>(L"Николай Гоголь «Мертвые души»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Эксмо")));
		session.addNew<Book>(L"Антон Чехов «Медведь»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Эксмо")));
		session.addNew<Book>(L"Уильям Шекспир, Сонеты", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Эксмо")));

		session.addNew<Book>(L"Иван Гончаров «Обломов»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Дрофа")));
		session.addNew<Book>(L"Михаил Лермонтов «Герой нашего времени»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Дрофа")));
		session.addNew<Book>(L"Александр Пушкин «Евгений Онегин»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Дрофа")));

		session.addNew<Book>(L"Иван Тургенев «Отцы и дети»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Лабиринт")));
		session.addNew<Book>(L"Лев Толстой «Война и мир»", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Лабиринт")));
		session.addNew<Book>(L"Михаил Зощенко, рассказы", session.find<Publisher>().where("name = ?").bind(Wt::WString(L"Лабиринт")));

		Wt::Dbo::collection<Wt::Dbo::ptr<Shop>> shop_coll = session.find<Shop>();
		for (const Wt::Dbo::ptr<Shop> &shop : shop_coll)//заполнение книг по магазинам псевдослучайным кол-вом
		{
			Wt::Dbo::collection<Wt::Dbo::ptr<Book>> book_coll = session.find<Book>();
			for (const Wt::Dbo::ptr<Book> &book : book_coll)
			{
				if (shop->name == L"Книжный у Кремля" && book->publisher->name == L"Эксмо")//убрал Эксмо из одного книжного
				{
					continue;
				}
				if ((shop->name == L"Книжный у Кремля" || shop->name == L"Книжный на Покровке") && book->publisher->name == L"Дрофа")//убрал Дрофа из двух книжных
				{
					continue;
				}

				int count = std::rand() % 5 + 10;
				session.addNew<Stock>(book, shop, count);				
			}
		}

		Wt::Dbo::collection<Wt::Dbo::ptr<Stock>> stock_coll = session.find<Stock>();
		for (const Wt::Dbo::ptr<Stock> &stock : stock_coll)//заполнение стоимостей, кол-ва и даты псевдослучайными числами
		{
			double price = std::rand() % 1000 + 100.99;
			int count = std::rand() % 3 + 1;
			std::string year = std::to_string(std::rand() % 23 + 2000);
			std::string month = std::to_string(std::rand() % 12 + 1);
			std::string day = std::to_string(std::rand() % 28 + 1);			
			std::string date_sale = year + "-" + month + "-" + day;
			session.addNew<Sale>(stock, price, count, date_sale);
		}
		transaction.commit();

		//запрос магазинов продающих книги определенного издательства
		std::string input = "exit";
		do
		{
			std::wstring wstr;
			std::wcout << L"Введите название издательства или exit для выхода: ";
			std::getline(std::cin, input);
			//преобразование в wstring под Windows
			size_t result_size = MultiByteToWideChar(1251, 0, input.c_str(), -1, 0, 0);
			wchar_t *w1 = new wchar_t[result_size];
			MultiByteToWideChar(1251, 0, input.c_str(), -1, w1, result_size);
			wstr = w1;
			delete w1;
			//вариант 2 преобразования в wstring, кол-во смиволов ограничено клавиатурой
			/*for (char &ch : input)
			{				
				if ('Ё' == ch)
				{
					wch = L'Ё';
				}
				else if ('ё' == ch)
				{
					wch = L'ё';
				}
				else if ('№' == ch)
				{
					wch = L'№';
				}
				else if (static_cast<uint8_t>(ch) >= 192)
				{
					wch = static_cast<wchar_t>(static_cast<int>(L'А') + (static_cast<int>(ch) - static_cast<int>('А')));
				}
				else
				{
					wch = static_cast<wchar_t>(ch);
				}
				wstr += wch;
			}*/
			//std::wcout << L"Вы ввели: " << wstr << std::endl;
			
			if (wstr == L"exit") { break; }
			publisher_query(session, wstr);
		} while(true);	
		
	}
	catch (const Wt::Dbo::Exception &err)
	{
		std::cout << err.what() << std::endl;
	}
	
}


