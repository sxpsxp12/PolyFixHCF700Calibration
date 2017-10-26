/*********
** node_info���ڵ���Ϣ
** addr: ��ַ
** version:�汾
** range: ����(KPa)
***********/
CREATE TABLE IF NOT EXISTS node_info(
	sn VARCHAR(20) UNIQUE,
	addr int,
	version int,
	range int
);

/*************************
**full_payload:�����̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS full_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
);


/*************************
**four_fifth_payload:���֮�����̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS four_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**three_fifth_payload:���֮�����̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS three_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**two_fifth_payload:���֮�����̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS two_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**one_fifth_payload:���֮һ���̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS one_fifth_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 


/*************************
**zero_payload:�����̸�������
** temp:�¶�
** pressure:ʵ��ѹǿ
** ref_pressure:����ѹǿ
** div_pressure:ѹǿƫ��
**************************/
CREATE TABLE IF NOT EXISTS zero_payload(
	unix_time double UNIQUE,
	temp double,
	pressure double,
	ref_pressure double,
	div_pressure double
); 
