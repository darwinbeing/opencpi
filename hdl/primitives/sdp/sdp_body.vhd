library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all, ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all;
package body sdp is

function dws2header(dws : dword_array_t(0 to sdp_header_ndws-1)) return header_t is
  variable h : header_t;
  variable v : std_logic_vector((sdp_header_ndws * 32)-1 downto 0);
  variable n : natural := 0;
begin
  assert dws'length = sdp_header_ndws;
  for i in 0 to sdp_header_ndws-1 loop
    v(i*32+31 downto i*32) := dws(i);
  end loop;
  h.count := unsigned(v(n+h.count'left downto n)); n := n + h.count'length;
  h.op    := op_t'val(to_integer(unsigned(v(n+op_width-1 downto n))));   n := n + op_width;
  h.xid   := unsigned(v(n+h.xid'left downto n));   n := n + h.xid'length;
  h.lead  := unsigned(v(n+h.lead'left downto n));  n := n + h.lead'length;
  h.trail := unsigned(v(n+h.trail'left downto n));  n := n + h.trail'length;
  h.node  := unsigned(v(n+h.node'left downto n));  n := n + h.node'length;
  h.addr  := unsigned(v(n+h.addr'left downto n));
  return h;
end dws2header;

function header2dws(h : header_t) return dword_array_t is
  variable dws : dword_array_t(0 to sdp_header_ndws-1);
  variable v : unsigned((sdp_header_ndws * 32)-1 downto 0);
  variable n : natural := 0;
begin
  v(n+h.count'left downto n) := h.count; n := n + h.count'length;
  v(n+op_width-1 downto n) := to_unsigned(op_t'pos(h.op), op_width); n := n + op_width;
  v(n+h.xid'left downto n) := h.xid;   n := n + h.xid'length;
  v(n+h.lead'left downto n) := h.lead;  n := n + h.lead'length;
  v(n+h.trail'left downto n) := h.trail;  n := n + h.trail'length;
  v(n+h.node'left downto n) := h.node;  n := n + h.node'length;
  v(n+h.addr'left downto n) := h.addr;
  for i in 0 to sdp_header_ndws-1 loop
    dws(i) := slv(v(i*32+31 downto i*32));
  end loop;
  return dws;
end header2dws;

function header2be(h : header_t; word : unsigned) return std_logic_vector is
  variable be : unsigned(datum_bytes-1 downto 0) := (others => '1');
  variable mask : unsigned(datum_bytes-1 downto 0) := (others => '1');
begin
  if word = 0 then
    be := be and (mask sll to_integer(h.lead));
  end if;
  if word = h.count then
    be := be and (mask srl to_integer(h.trail));
  end if;
  return slv(be);
end header2be;

function count_in_dws(header : header_t) return unsigned is
begin
  return resize(header.count, width_for_max(max_message_units)) + 1;
end count_in_dws;
function dword2header(dw : dword_t) return header_t is
  variable da : dword_array_t(0 to sdp_header_ndws-1);
begin  
  da(0) := dw;
  return dws2header(da);
end dword2header;
function count_in_dws(dw : dword_t) return unsigned is
  variable da : dword_array_t(0 to sdp_header_ndws-1);
begin
  da(0) := dw;
  return count_in_dws(dword2header(dw));
end count_in_dws;
function payload_in_dws(hdr : header_t) return unsigned is
begin
  if hdr.op = read_e then
    return to_unsigned(0, width_for_max(max_message_units));
  else
    return count_in_dws(hdr);
  end if;
end payload_in_dws;
function payload_in_dws(dw : dword_t) return unsigned is
begin
  return payload_in_dws(dword2header(dw));
end payload_in_dws;
function start_dw(header : header_t; sdp_width : uchar_t) return natural is
begin
  if sdp_width = 1 then
    return 0;
  else
    return to_integer(header.addr(width_for_max(to_integer(sdp_width))-1 downto 0));
  end if;
end start_dw;
end sdp;
--function header2dws(header : header_t) return dword_array_t;
--function dws2header(dws : dword_array_t) return header_t;
--function length_dws(header : header_t) return unsigned;
--function length_dws(dw : dword_t) return natural;
--function offset_dws(header : header_t) return unsigned;