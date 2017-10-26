/*********
** node_info：节点信息
** addr: 地址
** version:版本
** range: 量程(KPa)
***********/
CREATE TABLE IF NOT EXISTS node_info(
	sn VARCHAR(20) UNIQUE,
	addr int,
	version int,
	range int
);

/*************************
**full_payload:满量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS full_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
);


/*************************
**four_fifth_payload:五分之四量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS four_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**three_fifth_payload:五分之三量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS three_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**two_fifth_payload:五分之二量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS two_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**one_fifth_payload:五分之一量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS one_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**zero_payload:零量程负载数据
** temp:温度
** pressure:实际压强
** ref_pressure:参照压强
** div_pressure:压强偏差
**************************/
CREATE TABLE IF NOT EXISTS zero_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 
