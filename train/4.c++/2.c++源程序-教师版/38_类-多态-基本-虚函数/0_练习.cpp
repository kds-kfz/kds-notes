练习1 : Student Teacher
		用一个数组[8] 存储Student和Teacher的对象
		Person* arr[8];
		arr[i] = new Student;
		if (arr[i]->isGood())
				arr[i]->show();
练习2 : 
基类:	 Person			
			id, name, Person* next;
			static Person* head;
			virtual void show() 
			virtual bool isGood() 			
派生: Teacher		
	  论文数量count
	  void show()
	  isGood() {
		论文数量大于3是优秀教师
	  }
派生:   Student
		score
		void show()
		isGood(){
			分数大于90
		}
	Person* head;
	new Student;
	new Teacher;


