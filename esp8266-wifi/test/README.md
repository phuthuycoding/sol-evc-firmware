# Quick Test Reference

## Run Tests

```bash
# Native (fast, on PC)
pio test -e native

# Embedded (on ESP8266 hardware)
pio test -e test_esp

# Specific test
pio test -e native -f test_uart_protocol
```

## Test Categories

- **test_handlers/** - Business logic (handlers) - Run on NATIVE
- **test_drivers/** - Hardware drivers - Run on ESP8266
- **test_protocol/** - UART protocol - Run on NATIVE
- **test_mocks/** - Mock objects for testing

## Quick TDD Workflow

```bash
# 1. Write test
vim test/test_handlers/test_my_feature.cpp

# 2. Run (should fail RED)
pio test -e native -f test_my_feature

# 3. Implement
vim src/handlers/my_feature.cpp

# 4. Run (should pass GREEN)
pio test -e native -f test_my_feature

# 5. Verify on hardware
pio test -e test_esp -f test_my_feature
```

See **TEST_GUIDE.md** for full documentation.
