function tx(data)
   return string.char(0xAA) .. data .. string.char(0xFF)
end
-- Funkce rx() chybí -> příchozí data se zobrazí normálně (textově).
