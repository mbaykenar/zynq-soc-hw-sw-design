library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library STD;
use std.env.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

use uvvm_util.axistream_bfm_pkg.all;

use std.textio.all;
use std.env.finish;

entity tb_coprocessor_axistream_v1_0 is
end tb_coprocessor_axistream_v1_0;

architecture sim of tb_coprocessor_axistream_v1_0 is

-----------------------------------------------------------------------------
-- CONSTANTS
-----------------------------------------------------------------------------
-- Clock Constants
constant c_clkperiod                : time := 10 ns;
constant c_clkfreq                  : integer := 100_000_000;
constant c_clock_high_percentage    : integer := 50;
-- AXI-Stream Constants
constant c_axistream_data_width     : integer := 64;

constant c_axifull_id_width         : integer := 1;
constant c_axifull_user_width       : integer := 1;
constant c_axifull_base_addr        : integer := 0;
-- UART constants
constant c_baudrate                 : integer := 115_200;

-----------------------------------------------------------------------------
-- GLBOAL I/O Signals
		-- Ports of Axi Slave Bus Interface S00_AXIS
signal 		s00_axis_aclk	: std_logic := '0';
signal 		s00_axis_aresetn: std_logic := '1';
signal 		s00_axis_tready	: std_logic;
signal 		s00_axis_tdata	: std_logic_vector(C_S00_AXIS_TDATA_WIDTH-1 downto 0) := (others => '0');
signal 		s00_axis_tlast	: std_logic := '0';
signal 		s00_axis_tvalid	: std_logic := '0';

		-- Ports of Axi Master Bus Interface M00_AXIS
--signal 		m00_axis_aclk	: std_logic := '0';
--signal 		m00_axis_aresetn: std_logic := '1';
signal 		m00_axis_tvalid	: std_logic;
signal 		m00_axis_tdata	: std_logic_vector(C_M00_AXIS_TDATA_WIDTH-1 downto 0);
signal 		m00_axis_tlast	: std_logic;
signal 		m00_axis_tready	: std_logic := '0';

-----------------------------------------------------------------------------
-- AXISTREAM_BFM Signals
subtype t_axis is t_axistream_if(
tdata(C_S00_AXIS_TDATA_WIDTH-1 downto 0), tkeep(0 downto 0),
tuser(0 downto 0), tstrb(0 downto 0),
tid(0 downto 0), tdest( 0 downto 0));

signal m_axis             : t_axis;
signal s_axis             : t_axis;
signal axistream_bfm_config     : t_axistream_bfm_config := C_AXISTREAM_BFM_CONFIG_DEFAULT;

begin

-----------------------------------------------------------------------------
-- INSTANTIATIONS
-----------------------------------------------------------------------------
DUT : entity work.coprocessor_axistream_v1_0
generic map(
		-- Parameters of Axi Slave Bus Interface S00_AXIS
		C_S00_AXIS_TDATA_WIDTH	=> 64,

		-- Parameters of Axi Master Bus Interface M00_AXIS
		C_M00_AXIS_TDATA_WIDTH	=> 64
)
port map(
    s00_axis_aclk	    => s00_axis_aclk,
    s00_axis_aresetn    => s00_axis_aresetn,
    s00_axis_tready	    => s_axis.tready,
    s00_axis_tdata	    => s_axis.tdata,
    s00_axis_tlast	    => s_axis.tlast,
    s00_axis_tvalid	    => s_axis.tvalid,

    -- Ports of Axi Master Bus Interface M00_AXIS
    m00_axis_aclk	    => s00_axis_aclk,
    m00_axis_aresetn    => s00_axis_aresetn,
    m00_axis_tvalid	    => m_axis.tvalid,
    m00_axis_tdata	    => m_axis.tdata,
    m00_axis_tlast	    => m_axis.tlast,
    m00_axis_tready	    => m_axis.tready
);

-----------------------------------------------------------------------------
-- Clock Generator
-----------------------------------------------------------------------------
clock_generator(s00_axis_aclk, c_clkperiod, c_clock_high_percentage);

------------------------------------------------
-- PROCESS: p_main
------------------------------------------------
p_main: process
    constant C_SCOPE        : string    := C_TB_SCOPE_DEFAULT;
    variable v_time_stamp   : time      := 0 ns;
    variable v_data_array   : std_logic_vector(10*c_axistream_data_width-1 downto 0);

    variable v_data_axifull : t_slv_array(0 to 0)(c_axifull_data_width-1 downto 0);
    variable v_addr_axifull : unsigned(c_axifull_addr_width- 1 downto 0);    
    variable v_wstrb_value  : t_slv_array(0 to 0)(3 downto 0);
    variable v_wuser_value  : t_slv_array(0 to 0)(0 downto 0);

    variable v_buser_value : std_logic_vector(c_axifull_user_width-1 downto 0);
    variable v_bresp_value : uvvm_util.axi_bfm_pkg.t_xresp;
    variable v_awlen_value : unsigned(7 downto 0);
  
    variable v_exp_data_array    : std_logic_vector(9*c_axistream_data_width-1 downto 0);    

    -----------------------------------------------------------------------------
    -- AXI-Stream Internal Procedures    
    procedure axistream_transmit (
        constant data_array   : in    std_logic_vector;
        constant msg          : in    string) is
    begin

        axistream_transmit (
            data_array,
            msg,
            s00_axis_aclk,
            s_axis,
            C_SCOPE,               
            shared_msg_id_panel,  
            axistream_bfm_config    
        );

    end;


    procedure axistream_expect (
        constant exp_data_array : in    std_logic_vector;  -- Expected data
        constant msg            : in    string) is
    begin

        axistream_expect (
            exp_data_array,
            msg,
            s00_axis_aclk,
            m_axis,
            C_SCOPE,               
            shared_msg_id_panel,  
            axistream_bfm_config    
            );        

    end;  

    -----------------------------------------------------------------------------
    -- BEGIN STIMULI
    -----------------------------------------------------------------------------
begin

    -- AXI-Stream initializations
    axistream_bfm_config.clock_period     <= c_clkperiod;
    wait for 1 ps;


    -- Print the configuration to the log
    -- report_global_ctrl(VOID);
    -- report_msg_id_panel(VOID);

    --enable_log_msg(ALL_MESSAGES);
    disable_log_msg(ALL_MESSAGES);
    --enable_log_msg(ID_LOG_HDR);

    log(ID_LOG_HDR, "Start Simulation of TB for custom AXI-Stream IP", C_SCOPE);

    -- release active-low resetn signal 
    s00_axis_aresetn  <= '0';
    wait for c_clkperiod*10;
    s00_axis_aresetn  <= '1';
    wait for c_clkperiod*10;

    -- initialize AXI-Stream signals with functions
    m_axis      <= init_axistream_if_signals(
        true,                       -- is_master   : boolean;  -- When true, this BFM drives data signals
        c_axistream_data_width,     -- data_width  : natural;
        0,                          -- user_width  : natural;
        0,                          -- id_width    : natural;
        0,                          -- dest_width  : natural;
        axistream_bfm_config        --config      : t_axistream_bfm_config := C_AXISTREAM_BFM_CONFIG_DEFAULT
        );

    s_axis      <= init_axistream_if_signals(
        false,                      -- is_master   : boolean;  -- When true, this BFM drives data signals
        c_axistream_data_width,     -- data_width  : natural;
        0,                          -- user_width  : natural;
        0,                          -- id_width    : natural;
        0,                          -- dest_width  : natural;
        axistream_bfm_config        --config      : t_axistream_bfm_config := C_AXISTREAM_BFM_CONFIG_DEFAULT
        );        
    wait for c_clkperiod;

    v_exp_data_array    <=  x"0000000000000002" &
                            x"0000000000000004" &
                            x"0000000000000006" &
                            x"0000000000000008" &
                            x"000000000000000A" &
                            x"000000000000000C" &
                            x"000000000000000E" &
                            x"0000000000000010" &
                            x"0000000000000012";

                            
    axistream_expect(
        s00_axis_aclk   => s00_axis_aclk,
        exp_data_array  => v_exp_data_array,
        s_axis          => s_axis
    );

    v_data_array    <=  x"0000000000BACD00" &
                        x"0000000100000001" &
                        x"0000000200000002" &
                        x"0000000300000003" &
                        x"0000000400000004" &
                        x"0000000500000005" &
                        x"0000000600000006" &
                        x"0000000700000007" &
                        x"0000000800000008" &
                        x"0000000900000009";

    axistream_transmit(
        s00_axis_aclk   => s00_axis_aclk,
        v_data_array    => v_data_array,
        m_axis          => m_axis
    );

    wait for c_clkfreq*20;
    wait for 1 ps;

    --==================================================================================================
    -- Ending the simulation
    --------------------------------------------------------------------------------------
    wait for 1000 ns;             -- to allow some time for completion
    report_alert_counters(FINAL); -- Report final counters and print conclusion for simulation (Success/Fail)
    log(ID_LOG_HDR, "SIMULATION COMPLETED", C_SCOPE);

    -- Finish the simulation
    std.env.stop;
    wait;  -- to stop completely

end process p_main;

end architecture;