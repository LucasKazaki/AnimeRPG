// Compiled into every native test target, never into AstralGame.
// Release tests must evaluate assert expressions, including their side effects.
#ifdef NDEBUG
#error "Astral test assertions are disabled. Register this target with astral_add_test."
#endif
