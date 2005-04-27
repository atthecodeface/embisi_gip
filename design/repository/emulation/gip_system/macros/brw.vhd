LIBRARY ieee;
USE ieee.std_logic_1164.ALL;

ENTITY brw IS
	port (
	rom_address: IN std_logic_VECTOR(11 downto 0);
	rom_clock: IN std_logic;
	rom_read_data: OUT std_logic_VECTOR(31 downto 0);
	rom_read: IN std_logic);
END brw;

ARCHITECTURE brw_a of brw IS

component boot_rom
	port (
	addr: IN std_logic_VECTOR(11 downto 0);
	clk: IN std_logic;
	dout: OUT std_logic_VECTOR(31 downto 0);
	en: IN std_logic);
end component;

BEGIN

U0 : boot_rom
		port map (
			addr => rom_address,
			clk => rom_clock,
			dout => rom_read_data,
			en => rom_read);
END brw_a;
