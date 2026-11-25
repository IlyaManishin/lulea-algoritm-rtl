#pragma once

namespace engine
{
    class ILookupEngineFactory;
}

namespace tests
{
    void verify_lookup_engine(engine::ILookupEngineFactory &engine_factory);
} // namespace tests
