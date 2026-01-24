-- Funkce rx() pro normální data
function rx(data)
    return data 
end

-- NOVÁ FUNKCE: Volá se automaticky při změně velikosti okna
function on_resize(rows, cols)
    -- Vytvoříme informační řádek
    local msg = string.format("\r\n[INFO] New Size: %d x %d\r\n", rows, cols)
    
    -- Vrátíme ho -> C++ ho hned vypíše do terminálu
    return msg
end
