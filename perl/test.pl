#!/usr/bin/env perl
use Encode;
use JSON;

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

# 列表
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

# 数组
@A = qw(how are you);
print @A;  #没有空格
print "\n@A\n";  #元素之间有空格
print scalar @A;  #输出数组元素个数
print "\n";

# 数组下标访问
@a = qw(li zhi xin);
print $a[0];
print $a[1];
print "\n";
$a[2] = "wei";
print @a;
print "\n";

# 数组划分成切片
@a = qw(how are you 12 10 99 # ! @ ^ *);
@b = @a[2, 4, 6];
print "@b\n";

# 返回数组结尾的下标
print "$#a\n";

# 返回数组的个数
$size = @a;
print "$size\n";

# 倒数第3个元素
print "@a[-3]\n";

# 判断数组是否为空
if (@a){
    print "Not empty\n";
}

@star = ('*') x 10;
foreach (@star){
    print "$_";
}

print "\n";
print scalar(localtime);
print "\n";

#($a) = <STDIN>;
#print "$a\n";

# 普通循环
@name = qw(li zhi xin);
print "my name is ";
for ($index = 0; $index < @name; $index++){
        print "@name[$index] ";
}
print "\n";

# 拥有一个模式和一个标量，可以使用模式来分割标量
@a = qw(how are you);
@b = split(' ',"how are you");
foreach (@b){
    print "$_";
}
print "\n";

# 去除一个字符串和一个数组（列表），使用该字符串将数组中的元素连接起来
$num = join('@',(1..10));
print "$num\n";

@numbers=(1,5,2,7,3,8,4);
@sorted=sort {return(1) if($a>$b);
    return(0) if($a==$b);
    return(-1) if($a<$b);
} @numbers;
print "@sorted\n";

@numbers=(1,5,2,7,3,8,4);
@sorted=sort {$a <=> $b} @numbers; #a，b的顺序决定排序的类型
print "@sorted\n";

@lines = qw(li zhi xin);
print join(' ', reverse @lines);
print "\n";

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

# die 直接终端程序
open(MYTEXT, "novel.txt") or die "Cannot open myfile: $!\n"; 

# 循环读取每行内容 文件读完返回undef
# defind()函数，检查表达式的值是否是undef
#while(defined($line = <MYTEXT>)){
#    print "$line";
#}

# 统计循环次数
$conunt = 0;
while(<MYTEXT>){
    $count += 1;
    print "$count";
    print "$_";
}
close(MYTEXT);

# 以只读形式打开文件，小于< signe 指示，文件必须以只读模式运行结束
$conunt = 0;
open(MYTEXT, "<novel.txt");
while(<MYTEXT>){
    $count ++;
    print "$count";
    print "$_";
}
close(MYTEXT);

# 写入并覆盖
# open(NEWTH, ">output.txt") or die "Opening output.txt: $!";
# 写入并追加
# open(APPFH, ">>logfile.txt") or "Opening logfile.txt: $!";

# 用warn取代die，程序不停止，只发出警告。
if (!open(MYFILE, "output")){
    warn "Cannot read output: $!\n";
}else{
    # 执行代码
}

print "可以执行到这\n";

# 写入并覆盖
#open(NEWTH, ">output.txt") or die "Opening output.txt: $!";
# 写入并追加
#open(APPFH, ">>logfile.txt") or "Opening logfile.txt: $!";

# 用print( )将数据写入文件句柄
#open(LOGF, ">>logfile") or die "$!";
#if (! print LOGF "Time", scalar(localtime), "\n"){
#        warn "Unable to write to the log file: $!";
#}
#close(LOGF);

# 文件测试
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

$json = JSON->new->utf8;
#验证 utf8 标志位
print "utf8 flag: ",$json->get_utf8,"\n";

#输入是hash
#my $hash = {1=>111, 2=>222,3=>333};
#print "scalar var: ", $json->encode($hash),"\n";

# 正则表达式

$number="ggqq/ptyw/login";
if ($number =~ /([a-zA-Z]+)\/([a-zA-Z]+)\/([a-zA-Z]+)/){
#    print "$1_$2_$3\n";
    my $hash = {"$3"=>"$1_$2_$3"};
    print "scalar var: ", $json->encode($hash),"\n";
}
=pod
@dogs=qw(greyhound bllodhound terrier mutt chihuahua);
@hound=grep /hound/, @dogs;
print "@hound\n";

@hound=grep length($_)>8, @dogs;
print "@hound\n";
=cut




