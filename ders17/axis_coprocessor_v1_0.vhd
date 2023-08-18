library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

entity axis_coprocessor_v1_0 is
	generic (
		-- Users to add parameters here

		-- User parameters ends
		-- Do not modify the parameters beyond this line


		-- Parameters of Axi Slave Bus Interface S00_AXI
		C_S00_AXI_DATA_WIDTH	: integer	:= 32;
		C_S00_AXI_ADDR_WIDTH	: integer	:= 4;

		-- Parameters of Axi Slave Bus Interface S00_AXIS
		C_S00_AXIS_TDATA_WIDTH	: integer	:= 64;

		-- Parameters of Axi Master Bus Interface M00_AXIS
		C_M00_AXIS_TDATA_WIDTH	: integer	:= 64
	);
	port (
		-- Users to add ports here

		-- User ports ends
		-- Do not modify the ports beyond this line


		-- Ports of Axi Slave Bus Interface S00_AXI
		s00_axi_aclk	: in std_logic;
		s00_axi_aresetn	: in std_logic;
		s00_axi_awaddr	: in std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
		s00_axi_awprot	: in std_logic_vector(2 downto 0);
		s00_axi_awvalid	: in std_logic;
		s00_axi_awready	: out std_logic;
		s00_axi_wdata	: in std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
		s00_axi_wstrb	: in std_logic_vector((C_S00_AXI_DATA_WIDTH/8)-1 downto 0);
		s00_axi_wvalid	: in std_logic;
		s00_axi_wready	: out std_logic;
		s00_axi_bresp	: out std_logic_vector(1 downto 0);
		s00_axi_bvalid	: out std_logic;
		s00_axi_bready	: in std_logic;
		s00_axi_araddr	: in std_logic_vector(C_S00_AXI_ADDR_WIDTH-1 downto 0);
		s00_axi_arprot	: in std_logic_vector(2 downto 0);
		s00_axi_arvalid	: in std_logic;
		s00_axi_arready	: out std_logic;
		s00_axi_rdata	: out std_logic_vector(C_S00_AXI_DATA_WIDTH-1 downto 0);
		s00_axi_rresp	: out std_logic_vector(1 downto 0);
		s00_axi_rvalid	: out std_logic;
		s00_axi_rready	: in std_logic;

		-- Ports of Axi Slave Bus Interface S00_AXIS
		s00_axis_aclk	: in std_logic;
		s00_axis_aresetn: in std_logic;
		s00_axis_tready	: out std_logic;
		s00_axis_tdata	: in std_logic_vector(C_S00_AXIS_TDATA_WIDTH-1 downto 0);
		s00_axis_tlast	: in std_logic;
		s00_axis_tvalid	: in std_logic;

		-- Ports of Axi Master Bus Interface M00_AXIS
		m00_axis_aclk	: in std_logic;
		m00_axis_aresetn: in std_logic;
		m00_axis_tvalid	: out std_logic;
		m00_axis_tdata	: out std_logic_vector(C_M00_AXIS_TDATA_WIDTH-1 downto 0);
		m00_axis_tlast	: out std_logic;
		m00_axis_tready	: in std_logic
	);
end axis_coprocessor_v1_0;

architecture arch_imp of axis_coprocessor_v1_0 is

	-- component declaration
	component axis_coprocessor_v1_0_S00_AXI is
		generic (
		C_S_AXI_DATA_WIDTH	: integer	:= 32;
		C_S_AXI_ADDR_WIDTH	: integer	:= 4
		);
		port (
		-- MBA START
		operation		: out std_logic_vector(1 downto 0);
		-- MBA END
		S_AXI_ACLK		: in std_logic;
		S_AXI_ARESETN	: in std_logic;
		S_AXI_AWADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
		S_AXI_AWPROT	: in std_logic_vector(2 downto 0);
		S_AXI_AWVALID	: in std_logic;
		S_AXI_AWREADY	: out std_logic;
		S_AXI_WDATA		: in std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
		S_AXI_WSTRB		: in std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
		S_AXI_WVALID	: in std_logic;
		S_AXI_WREADY	: out std_logic;
		S_AXI_BRESP		: out std_logic_vector(1 downto 0);
		S_AXI_BVALID	: out std_logic;
		S_AXI_BREADY	: in std_logic;
		S_AXI_ARADDR	: in std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
		S_AXI_ARPROT	: in std_logic_vector(2 downto 0);
		S_AXI_ARVALID	: in std_logic;
		S_AXI_ARREADY	: out std_logic;
		S_AXI_RDATA		: out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
		S_AXI_RRESP		: out std_logic_vector(1 downto 0);
		S_AXI_RVALID	: out std_logic;
		S_AXI_RREADY	: in std_logic
		);
	end component axis_coprocessor_v1_0_S00_AXI;

	-- MBA START
	signal operation 	: std_logic_vector (1 downto 0) := "00";
	signal result		: std_logic_vector (8*8-1 downto 0) := (others => '0');
	signal add_result	: std_logic_vector (8*8-1 downto 0) := (others => '0');
	signal sub_result	: std_logic_vector (8*8-1 downto 0) := (others => '0');
	signal mul_result	: std_logic_vector (8*8-1 downto 0) := (others => '0');	
	-- MBA END

begin

-- Instantiation of Axi Bus Interface S00_AXI
axis_coprocessor_v1_0_S00_AXI_inst : axis_coprocessor_v1_0_S00_AXI
	generic map (
		C_S_AXI_DATA_WIDTH	=> C_S00_AXI_DATA_WIDTH,
		C_S_AXI_ADDR_WIDTH	=> C_S00_AXI_ADDR_WIDTH
	)
	port map (
		-- MBA START
		operation	=> operation,
		-- MBA END
		S_AXI_ACLK	=> s00_axi_aclk,
		S_AXI_ARESETN	=> s00_axi_aresetn,
		S_AXI_AWADDR	=> s00_axi_awaddr,
		S_AXI_AWPROT	=> s00_axi_awprot,
		S_AXI_AWVALID	=> s00_axi_awvalid,
		S_AXI_AWREADY	=> s00_axi_awready,
		S_AXI_WDATA	=> s00_axi_wdata,
		S_AXI_WSTRB	=> s00_axi_wstrb,
		S_AXI_WVALID	=> s00_axi_wvalid,
		S_AXI_WREADY	=> s00_axi_wready,
		S_AXI_BRESP	=> s00_axi_bresp,
		S_AXI_BVALID	=> s00_axi_bvalid,
		S_AXI_BREADY	=> s00_axi_bready,
		S_AXI_ARADDR	=> s00_axi_araddr,
		S_AXI_ARPROT	=> s00_axi_arprot,
		S_AXI_ARVALID	=> s00_axi_arvalid,
		S_AXI_ARREADY	=> s00_axi_arready,
		S_AXI_RDATA	=> s00_axi_rdata,
		S_AXI_RRESP	=> s00_axi_rresp,
		S_AXI_RVALID	=> s00_axi_rvalid,
		S_AXI_RREADY	=> s00_axi_rready
	);

	-- Add user logic here
-- MBA START

--==========================================================================
-- Combinational assignments
--==========================================================================
result 	<= 	add_result when operation = "00" else
			sub_result when operation = "01" else
			mul_result when operation = "10" else
			add_result;

add_result(31 downto 0) 		<= 	s00_axis_tdata(8*8-1 downto 4*8) + s00_axis_tdata(4*8-1 downto 0*8);
add_result(8*8-1 downto 4*8) 	<= (others => (add_result(31)));
sub_result(31 downto 0) 		<= 	s00_axis_tdata(8*8-1 downto 4*8) - s00_axis_tdata(4*8-1 downto 0*8);
sub_result(8*8-1 downto 4*8) 	<= (others => (sub_result(31)));
mul_result 						<= s00_axis_tdata(8*8-1 downto 4*8) * s00_axis_tdata(4*8-1 downto 0*8);

--==========================================================================
-- TREADY is always '1' if the receiver is ready
--==========================================================================
P_TREADY : process (s00_axis_aclk)
begin
if rising_edge(s00_axis_aclk) then
	if s00_axis_aresetn = '0' then
		s00_axis_tready	<= '0';
	elsif (m00_axis_tready = '1') then
		s00_axis_tready	<= '1';
	else 
		s00_axis_tready	<= '0';
	end if;
end if;
end process P_TREADY;

--==========================================================================
-- Master TLAST follows slave TLAST 
--==========================================================================
P_TLAST : process (s00_axis_aclk)
begin
if rising_edge(s00_axis_aclk) then
	if s00_axis_aresetn = '0' then
		m00_axis_tlast	<= '0';
	elsif (s00_axis_tlast = '1') then
		m00_axis_tlast	<= '1';
	else 
		m00_axis_tlast	<= '0';
	end if;
end if;
end process P_TLAST;

--==========================================================================
-- Master TVALID follows slave TVALID 
--==========================================================================
P_TVALID : process (s00_axis_aclk)
begin
if rising_edge(s00_axis_aclk) then
	if s00_axis_aresetn = '0' then
		m00_axis_tvalid	<= '0';
	elsif (s00_axis_tvalid = '1'and m00_axis_tready = '1') then
		m00_axis_tvalid	<= '1';
	else 
		m00_axis_tvalid	<= '0';
	end if;
end if;
end process P_TVALID;



--==========================================================================
-- Master TDATA is assigned to result with clock
--==========================================================================
P_TDATA : process (s00_axis_aclk)
begin
if rising_edge(s00_axis_aclk) then
	if s00_axis_aresetn = '0' then
		m00_axis_tdata	<= (others => '0');
	else
		m00_axis_tdata 	<= result;
	end if;
end if;
end process P_TDATA;

-- MBA END
	-- User logic ends

end arch_imp;
