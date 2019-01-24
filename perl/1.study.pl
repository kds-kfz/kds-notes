#!/usr/bin/env perl
=pod
    作者：kfz
    日期：2019-01-24
=cut

use Encode;
use JSON;
use Cwd;

# -- 第 1 节 here 文档 --
$a = 10;
$var = <<"END";
这是一个 Here 文档实例，使用双引号。
可以在这输如字符串和变量。
例如：a = $a
END
print "$var\n";
 
$var = <<'EOF';
这是一个 Here 文档实例，使用单引号。
例如：a = $a
EOF
print "$var\n";

# -- 第 2 节 列表 --
@a = (1..10);
@b = (1..10, 21..30);
#字符照样可以使用范围运算符
@list = (aa..zz);
#foreach $ss (@list){
#    print "$ss ";
#    }
# 等价于
#foreach (@list){
#    print "$_ ";
#}
print "@a\n";
print scalar @a;
print "\n";

# -- 第 3 节 数组 --
@A = qw(how are you);
print @A;  #没有空格
print "\n@A\n";  #元素之间有空格
print scalar @A;  #输出数组元素个数
print "\n";

# -- 第 4 节 数组下标访问 --
@a = qw(li zhi xin);
print $a[0];
print $a[1];
print "\n";
$a[2] = "wei";
print @a;
print "\n";

# -- 第 5 节 数组划分成切片 --
@a = qw(how are you 12 10 99 # ! @ ^ *);
@b = @a[2, 4, 6];
print "@b\n";

# -- 第 6 节 返回数组结尾的下标 --
print "$#a\n";

# -- 第 7 节 返回数组的个数 --
$size = @a;
print "$size\n";

# -- 第 8 节 倒数第3个元素 --
print "@a[-3]\n";

# -- 第 9 节 判断数组是否为空 --
if (@a){
    print "Not empty\n";
}

# -- 第 10 节 x号 --
@star = ('*') x 10;
foreach (@star){
    print "$_";
}
print "\n";

# -- 第 11 节 获取时间 --
print scalar(localtime);
print "\n";

#($a) = <STDIN>;
#print "$a\n";

# -- 第 12 节 for循环 --
@name = qw(li zhi xin);
print "my name is ";
for ($index = 0; $index < @name; $index++){
        print "@name[$index] ";
}
print "\n";

# -- 第 13 节 split 分割 -- 
# 拥有一个模式和一个标量，可以使用模式来分割标量
@a = qw(how are you);
@b = split(' ',"how are you");
foreach (@b){
    print "$_";
}
print "\n";

# -- 第 14 节 join 插入字符 --
# 去除一个字符串和一个数组（列表），使用该字符串将数组中的元素连接起来
$num = join('@',(1..10));
print "$num\n";

# -- 第 15 节 sort 排序 --
@numbers=(1,5,2,7,3,8,4);
@sorted=sort {return(1) if($a>$b);
    return(0) if($a==$b);
    return(-1) if($a<$b);
} @numbers;
print "@sorted\n";

@numbers=(1,5,2,7,3,8,4);
@sorted=sort {$a <=> $b} @numbers; #a，b的顺序决定排序的类型
print "@sorted\n";

# -- 第 16 节 reverse 用法 --
@lines = qw(li zhi xin);
print join(' ', reverse @lines);
print "\n";

# -- 第 17 节 open 文件操作 --
# 文件操作 打开失败返回 undef
if (open(MYFILE, "mydatafile.txt")){
    # 成功运行
    print "你好\n";
}else{
    print "Cannot open mydatafile!\n";
    exit 1;
}

#open(MYTEXT, "novel.txt.c") || die; 
#close(MYTEXT);

# -- 第 18 节 die 直接终端程序 --
open(MYTEXT, "novel.txt") or die "Cannot open myfile: $!\n"; 

# 循环读取每行内容 文件读完返回undef
# defind()函数，检查表达式的值是否是undef
#while(defined($line = <MYTEXT>)){
#    print "$line";
#}

# -- 第 19 节 统计循环次数 --
$conunt = 0;
while(<MYTEXT>){
    $count += 1;
    print "$count";
    print "$_";
}
close(MYTEXT);

# -- 第 20 节 只读打开 --
# 以只读形式打开文件，小于< signe 指示，文件必须以只读模式运行结束
$conunt = 0;
open(MYTEXT, "<novel.txt");
while(<MYTEXT>){
    $count ++;
    print "$count";
    print "$_";
}
close(MYTEXT);

# -- 第 21 节 写入并覆盖 --
# open(NEWTH, ">output.txt") or die "Opening output.txt: $!";
# -- 第 22 节 写入并追加 --
# open(APPFH, ">>logfile.txt") or "Opening logfile.txt: $!";

# -- 第 23 节 warn 警告 --
# 用warn取代die，程序不停止，只发出警告。
if (!open(MYFILE, "output")){
    warn "Cannot read output: $!\n";
}else{
    # 执行代码
}
print "\n";

# -- 第 24 节 print 输出到文件 --
# 用print( )将数据写入文件句柄
#open(LOGF, ">>logfile") or die "$!";
#if (! print LOGF "Time", scalar(localtime), "\n"){
#        warn "Unable to write to the log file: $!";
#}
#close(LOGF);

# -- 第 25 节 warn 文件测试 --
# -e  #文件存在，则真
# -d  #是个目录，则真
# -r  #文件可读，则真
# -T  #文本文件，则真
# -B  #二进制文件，则真

$filename = 'novel.txt';
if (-s $filename){
    warn "$filename contents will be overwritten!\n";
    warn "$filename was last updated", -M $filename, "days ago.\n";
}

# -- 第 26 节 指定 json 编码 --
$json = JSON->new->utf8;
#验证 utf8 标志位
print "utf8 flag: ",$json->get_utf8,"\n";

# -- 第 27 节 hash 转 json --
my $hash = {1=>111, 2=>222,3=>333};
print "scalar var: ", $json->encode($hash),"\n";

# -- 第 28 节 hash 数组转 json --
#输入是hash数组，传入的是数组引用
my @hash = ();
push @hash ,{1=>111};
push @hash ,{1=>222};
push(@hash,{2=>333});
print "array ref: ", $json->encode(\@hash),"\n";

# -- 第 29 节 hash 数组转 json --
my %hash = ();
$hash{"rzrq/ptjy"}{"cccx"}="rzrq_ptjy_cccx";
$hash{"rzrq/ptjy"}{"cccx2"}="rzrq_ptjy_cccx2";
print "array ref: ", $json->encode(\%hash),"\n";

# -- 第 30 节 正则表达式 --
$number="ggqq/ptyw/login";
if ($number =~ /([a-zA-Z]+)\/([a-zA-Z]+)\/([a-zA-Z]+)/){
#    print "$1_$2_$3\n";
    my $hash = {"$3"=>"$1_$2_$3"};
    print "scalar var: ", $json->encode($hash),"\n";
}

# -- 第 31 节 pod 与 cut 注释 --
=pod
@dogs=qw(greyhound bllodhound terrier mutt chihuahua);
@hound=grep /hound/, @dogs;
print "@hound\n";

@hound=grep length($_)>8, @dogs;
print "@hound\n";
=cut


# -- 第 32 节 数组 hash 转 json --
my @arr =();
%hash_1 = (
        "login"=>"ggqq_ptyw_login",
        "zjcx"=>"ggqq_ptyw_zjcx"
        );

# -- 第 33 节 数组 列表 hash 转 json --
@hash_2 = (
        ["login"=>"ggqq_ptyw_login"],
        ["zjcx"=>"ggqq_ptyw_zjcx"]
        );
push @arr ,{%hash_1};
push @arr ,{@hash_2};
print "array ref: ", $json->encode(\@arr),"\n";

# -- 第 34 节 遍历 数组 列表 hash 转 json --
for(@hash_2){
    print "@{$_}\n";
}
print "array ref: ", $json->encode(\@hash_2),"\n";

# -- 第 35 节 二维数组 --
@AoA=(  
        ["fred","barney"],  
        ["george","jane","elroy"],  
        ["homer","marge","bart"],  
     );  
print "$AoA[2][2]\n";

#@AoA={"ggqq/ptyw"=>{
#                    "login"=>"ggqq_ptyw_login",
#                    "zjcx"=>"ggqq_ptyw_zjcx"
#                    }
#    };  

# -- 第 36 节 数组 hash 列表 hash --
%AoA=("ggqq/ptyw"=>[{
                    "login"=>"ggqq_ptyw_login",
                    "zjcx"=>"ggqq_ptyw_zjcx"
                    }]
     );  
#print "$AoA{"ggqq/ptyw"}{"login"}";
$AoA{"rzrq/ptyw"}[0]{"cccx"} = "ggqq_ptyw_cccx";
print "array ref here : ", $json->encode(\%AoA),"\n";

# -- 第 37 节 遍历 hash 列表 hash --
#for $k ( keys %AoA ) {
#    print "here $k: ";
#    for $role ( keys %{ $AoA{$k}[0] } ) {
#        print "$role=$AoA{$k}[0]{$role} ";
#    }
#    print "\n";
#}

# -- 第 37 节 获取绝对路径 --
my $getpath = getcwd;
print "$getpath\n";

