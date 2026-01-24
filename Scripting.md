# debuQterm Scripting Guide

debuQterm allows you to load Lua scripts to filter data, automate tasks, or format output in real-time. Each time a script is loaded, the Lua state is reset.

## global Variables

The application automatically sets the following global variables when the script is loaded or the window is resized:

* `TERM_ROWS` (int): Current number of terminal rows.
* `TERM_COLS` (int): Current number of terminal columns.

---

## API Functions (Call from Lua)

These functions are provided by the host application to interact with the GUI.

### `print_status(message, timeout_ms)`
Displays a message in the application status bar.
* `message`: String to display.
* `timeout_ms`: Duration in milliseconds (0 = persistent).

### `print_log(data)`
Writes data directly to the terminal window (Local Echo). This data is **not** sent to the serial port.
* `data`: String or binary data to display. Supports ANSI escape codes for colors.

---

## Event Hooks (Define in Lua)

Define these functions in your script to handle events. If a function is not defined, the default behavior (pass-through) is used.

### `rx(data)`
Called when data is received from the serial port.
* **Input:** Raw binary string received from hardware.
* **Return:** String to display in the terminal.
* **Default:** Returns `data` unchanged.

### `tx(data)`
Called when the user types or pastes text into the terminal.
* **Input:** Raw string from keyboard/clipboard.
* **Return:** String to send to the serial port.
* **Default:** Returns `data` unchanged.

### `on_tick(delta_ms)`
Called periodically by the application timer (approx. every 100ms).
* **Input:** Time elapsed since last tick in milliseconds.
* **Return:** String to **send to the serial port** (or `nil` to send nothing).
* **Use case:** Periodic polling, timeouts, blinking patterns.

### `on_resize(rows, cols)`
Called when the terminal window size changes.
* **Input:** New dimensions of the terminal.
* **Return:** String to **print to the terminal** (Local Echo).
* **Use case:** adjusting formatting limits or notifying the user.

---

## Code Examples

### 1. Basic Pass-Through (Default Behavior)
```lua
function rx(data) return data end
function tx(data) return data end
```
### 2. Hex Dump (Responsive)
```lua
local CHARS_PER_BYTE = 3 -- "XX "
local bytes_per_line = 16

local function recalc_limit(cols)
    if not cols then return 16 end
    local w = math.floor(cols / CHARS_PER_BYTE)
    if w < 1 then w = 1 end
    return w
end

-- Initialize based on current size
if TERM_COLS then bytes_per_line = recalc_limit(TERM_COLS) end

function on_resize(rows, cols)
    bytes_per_line = recalc_limit(cols)
    return "" -- Silent update
end

local col_counter = 0

function rx(data)
    local result = {}
    for i = 1, #data do
        local byte = string.byte(data, i)
        table.insert(result, string.format("%02X ", byte))
        
        col_counter = col_counter + 1
        if col_counter >= bytes_per_line then
            table.insert(result, "\r\n")
            col_counter = 0
        end
    end
    return table.concat(result)
end
```
### 3. Periodic Keep-Alive (Ping)
```lua
local timer = 0
local INTERVAL = 5000 -- 5 seconds

function on_tick(delta_ms)
    timer = timer + delta_ms
    
    if timer >= INTERVAL then
        timer = 0
        print_status("Sending Keep-Alive...", 1000)
        return "PING\r\n" -- Send to Serial Port
    end
    
    return nil
end
```
### 4. ANSI Colors Helper
```lua
local C = {
    RED = "\x1B[31m", GREEN = "\x1B[32m", YELLOW = "\x1B[33m",
    RESET = "\x1B[0m"
}

function rx(data)
    -- Highlight specific keywords
    if string.find(data, "ERROR") then
        return C.RED .. data .. C.RESET
    elseif string.find(data, "OK") then
        return C.GREEN .. data .. C.RESET
    end
    return data
end
```