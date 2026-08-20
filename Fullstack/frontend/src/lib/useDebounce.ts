import { useEffect, useState } from "react";

// Delays updating the returned value until `delayMs` has passed without
// `value` changing again. Used to avoid firing a network request on
// every keystroke -- spec section 18 asks for ~200-300ms.
export function useDebounce<T>(value: T, delayMs = 250): T {
  const [debounced, setDebounced] = useState(value);

  useEffect(() => {
    const timer = setTimeout(() => setDebounced(value), delayMs);
    return () => clearTimeout(timer);
  }, [value, delayMs]);

  return debounced;
}
