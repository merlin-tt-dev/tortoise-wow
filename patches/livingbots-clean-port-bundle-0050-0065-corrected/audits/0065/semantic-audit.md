# Semantic audit — 0065

## Native LLM singleton integration

- Adds the required explicit `SingletonImp.h` implementation include before `INSTANTIATE_SINGLETON_1(PlayerbotLLMInterface)`.
- No behavioral change beyond making the singleton instantiation compile under the host include model.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
