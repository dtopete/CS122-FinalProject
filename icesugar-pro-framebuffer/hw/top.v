`include "async_fifo.v"
`include "lcd_fb.v"
`include "lcd_timing.v"
`include "pll_clocks.v"
`include "sdram_controller.v"
`include "display_controller.v"

module top (
    // iCESugar-Pro 25MHz onboard clock (Pin P6)
    input  wire        clk_25m,       

    // IS42S16160B SDRAM Interface
    output wire        sdram_clk,
    output wire        sdram_cke,
    output wire        sdram_cs_n,
    output wire        sdram_ras_n,
    output wire        sdram_cas_n,
    output wire        sdram_we_n,
    output wire [1:0]  sdram_ba,
    output wire [12:0] sdram_a,
    output wire [1:0]  sdram_dqm,
    inout  wire [15:0] sdram_dq,

    // RGB LCD Interface (480x272)
    output wire        lcd_clk,
    output wire        lcd_hsync,
    output wire        lcd_vsync,
    output wire        lcd_de,
    output wire [7:0]  lcd_r,
    output wire [7:0]  lcd_g,
    output wire [7:0]  lcd_b,

    // SPI Peripheral for Display Commands from MCU
    input wire         sclk,
    input wire         pico,
    input wire         cs_n,
    input wire         data_cmd,

    output wire [7:0]  dbg,

    input wire         PPMinput, // PPM input from Pico
    output wire        ch2, // Outputs ch2's PWM
    output wire        ch3 // Outputs ch3's PWM
);

    reg        wr_en = 0;
    wire       wr_ack;
    reg [23:0] wr_addr = 24'h000000;       
    reg [15:0] wr_data = 16'h0000;

    icesugar_pro_lcd_fb fb_inst (
        .clk_25m(clk_25m),
        
        .sdram_clk(sdram_clk),
        .sdram_cke(sdram_cke),
        .sdram_cs_n(sdram_cs_n),
        .sdram_ras_n(sdram_ras_n),
        .sdram_cas_n(sdram_cas_n),
        .sdram_we_n(sdram_we_n),
        .sdram_ba(sdram_ba),
        .sdram_a(sdram_a),
        .sdram_dqm(sdram_dqm),
        .sdram_dq(sdram_dq),

        .lcd_clk(lcd_clk),
        .lcd_hsync(lcd_hsync),
        .lcd_vsync(lcd_vsync),
        .lcd_de(lcd_de),
        .lcd_r(lcd_r[7:3]),
        .lcd_g(lcd_g[7:2]),
        .lcd_b(lcd_b[7:3]),
        
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data),
        .wr_ack(wr_ack),
        .clk_100m(clk_100m),
        .locked(locked)
    );

    assign lcd_r[0:2] = {3{lcd_r[3]}};
    assign lcd_g[0:1] = {2{lcd_g[2]}};
    assign lcd_b[0:2] = {3{lcd_b[3]}};

    wire locked;
    wire reset = ~locked;
    wire clk_100m;
    wire [7:0]  pixel_half;
    wire [23:0] pixel_addr;
    wire        pixel_wr_en;

    display_controller # (.DISPLAY_WIDTH(960), .DISPLAY_HEIGHT(272)) controller_inst (
        .clk(clk_25m),
        .reset(reset),
        .disp_CS_n(cs_n),
        .disp_SCLK(sclk),
        .disp_PICO(pico),
        .data_cmd(data_cmd),
        .framebuffer_addr_out(pixel_addr),
        .framebuffer_data_out(pixel_half),
        .framebuffer_wren(pixel_wr_en)
    );

    assign dbg = {data_cmd, pico, sclk, cs_n};

    integer clock_count = 0;
    wire reset;

    always @(posedge clk_100m) begin
        if (~reset) begin
            wr_en <= wr_en & ~wr_ack;
            if (pixel_wr_en) begin
                if (~pixel_addr[0]) begin
                    wr_data[7:0] <= pixel_half;
                end else begin
                    wr_data[15:8] <= pixel_half;
                    wr_addr <= {0, pixel_addr[22:1]};
                    wr_en <= 1;
                end 
            end 
        end
    end

// PPM to PWM Decoder logic

// Synchronizer and Falling Edge Detector
reg ppm_sync1 = 1, ppm_sync2 = 1, ppm_sync3 = 1;
always @(posedge clk_25m) begin
    ppm_sync1 <= PPMinput;
    ppm_sync2 <= ppm_sync1;
    ppm_sync3 <= ppm_sync2;
end
wire ppm_falling_edge = ~ppm_sync2 & ppm_sync3;

// Decode PPM Stream
// At 25MHz, 1us = 25 cycles
// 4000 us sync width threshold = 100,000 cycles
localparam SYNC_THRESHOLD = 100_000;

reg [19:0] ppm_timer = 0;
reg [3:0] channel_idx = 0;

// Register holding pulse widths (clock cycles)
// Defaulting 1500us (1500 * 25 = 37_500 cycles) to keep servos centered on boot
reg [19:0] ch1_width = 37_500;
reg [19:0] ch2_width = 37_500;
reg [19:0] ch3_width = 37_500;
reg [19:0] ch4_width = 37_500;
reg [19:0] ch5_width = 37_500;
reg [19:0] ch6_width = 37_500;
reg [19:0] ch7_width = 37_500;
reg [19:0] ch8_width = 37_500;

always @(posedge clk_25m) begin
    if (ppm_falling_edge) begin
        if(ppm_timer >= SYNC_THRESHOLD) begin
            // Sync pulse detected, reset index to frame start
            channel_idx <= 0;
        end else begin
            // Channel 0 -> Ch1, channel 1 ->ch2, Channel 2 -> ch3
            if (channel_idx == 4'd0) ch2_width <= ppm_timer;
            if (channel_idx == 4'd1) ch2_width <= ppm_timer;
            if (channel_idx == 4'd2) ch3_width <= ppm_timer;
            if (channel_idx == 4'd3) ch4_width <= ppm_timer;
            if (channel_idx == 4'd4) ch5_width <= ppm_timer;
            if (channel_idx == 4'd5) ch6_width <= ppm_timer;
            if (channel_idx == 4'd6) ch7_width <= ppm_timer;
            if (channel_idx == 4'd7) ch8_width <= ppm_timer;

            channel_idx <= channel_idx + 1;
        end
        ppm_timer <= 0;
    end else begin
        // Increment timer, cap at ~40ms to prevent overflow during signal loss
        if(ppm_timer < 20'hFFFFF) begin
            ppm_timer <= ppm_timer + 1;
        end
    end
end

// Generate PWM Ouptut
// Standard PWM is 50Hz (20ms period)
// 20ms * 25MHz = 500_000 cycles
reg[18:0] pwm_counter = 0;

always @(posedge clk_25m) begin
    if(pwm_counter >= 19'd499999) begin
        pwm_counter <= 0;
    end else begin
        pwm_counter <= pwm_counter + 1;
    end
end

// Drive outputs high while counter is less than the decoded pulse width
// assign ch1 = (pwm_counter < ch1_width);
assign ch2 = (pwm_counter < ch2_width);
assign ch3 = (pwm_counter < ch3_width);
//assign ch4 = (pwm_counter < ch4_width);
//assign ch5 = (pwm_counter < ch5_width);
//assign ch6 = (pwm_counter < ch6_width);
//assign ch7 = (pwm_counter < ch7_width);
//assign ch8 = (pwm_counter < ch8_width);


endmodule