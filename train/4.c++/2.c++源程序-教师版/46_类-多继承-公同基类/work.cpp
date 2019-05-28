
某小型公司, 有四种员工，
	销售员，兼职技术员，经理，销售经理
	每个人都有自己的id，名字，级别，薪水
	
	抽象类
	class Employee {
		show() = 0;
		pay() = 0;
		double salary;
		做一个链表
	};
	class SalesMan : public Employee{
	};
	Technician : E <---虚继承
	Manager : E

	SalesManager : Manager, Salesman
	
	销售员	1: 以销售额的3%+2000	
	技术员	2: 每小时100 
	经理	3: 固定8000
	销售经理4: 所有销售额的5%+3000

