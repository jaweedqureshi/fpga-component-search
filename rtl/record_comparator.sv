module record_comparator (
    input  logic         clk,
    input  logic         rst_n,
    input  logic         in_valid,
    input  logic [511:0] record_data,
    input  logic [15:0]  category_query,
    input  logic [111:0] mpn_query,
    input  logic [111:0] mpn_mask,
    input  logic [63:0]  param0_min,
    input  logic [63:0]  param0_max,
    input  logic [63:0]  param1_min,
    input  logic [63:0]  param1_max,
    input  logic [63:0]  param2_min,
    input  logic [63:0]  param2_max,
    input  logic [31:0]  sku_query,
    input  logic [143:0] flags_value,
    input  logic [143:0] flags_mask,
    input  logic [15:0]  supplier_query,
    output logic         out_valid,
    output logic         out_match
);
    logic compare_now;
    logic stage1_valid;
    logic stage1_match;

    function automatic logic in_range(
        input logic [63:0] value_bits,
        input logic [63:0] min_bits,
        input logic [63:0] max_bits
    );
        real value;
        real min_value;
        real max_value;
        begin
            value = $bitstoreal(value_bits);
            min_value = $bitstoreal(min_bits);
            max_value = $bitstoreal(max_bits);
            in_range = (value >= min_value) && (value <= max_value);
        end
    endfunction

    always_comb begin
        compare_now =
            (record_data[511:496] == category_query) &&
            ((record_data[495:384] & mpn_mask) == (mpn_query & mpn_mask)) &&
            in_range(record_data[255:192], param0_min, param0_max) &&
            in_range(record_data[319:256], param1_min, param1_max) &&
            in_range(record_data[383:320], param2_min, param2_max) &&
            (record_data[191:160] == sku_query) &&
            ((record_data[159:16] & flags_mask) == (flags_value & flags_mask)) &&
            (record_data[15:0] == supplier_query);
    end

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            stage1_valid <= 1'b0;
            stage1_match <= 1'b0;
            out_valid <= 1'b0;
            out_match <= 1'b0;
        end else begin
            stage1_valid <= in_valid;
            stage1_match <= in_valid && compare_now;
            out_valid <= stage1_valid;
            out_match <= stage1_valid && stage1_match;
        end
    end
endmodule
