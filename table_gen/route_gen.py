from pathlib import Path

from core.mrt_gen import generate_mrt_routes
from core.gen_types import RouteRecord
from core.mrt_reader import MRTFileType
from core.random_gen import generate_random_routes

from config import GeneratorType, ROUTE_COUNT
from port_mapping import map_next_hops_to_ports,save_port_map

PORT_MAP_EXT = ".ports"


def generate_route_table(
    generator_type: GeneratorType,
    output_path: str | Path,
    input_path: str | Path | None = None,
    count: int = ROUTE_COUNT,
    seed: int | None = 42,
    mrt_file_type: MRTFileType = MRTFileType.TEXT,
) -> None:
    if generator_type == GeneratorType.MRT:
        if input_path is None:
            raise ValueError("input_path is required when generator_type is GeneratorType.MRT")
        routes: list[RouteRecord] = generate_mrt_routes(
            input_path=input_path,
            count=count,
            file_type=mrt_file_type,
            seed=seed,
        )
    elif generator_type == GeneratorType.RANDOM:
        routes: list[RouteRecord] = generate_random_routes(
            count=count,
            seed=seed,
        )
    else:
        raise ValueError(f"Unsupported generator type: {generator_type}")

    port_map = map_next_hops_to_ports(routes)

    port_map_path = Path(f"{output_path}{PORT_MAP_EXT}")
    save_port_map(port_map, port_map_path)

    filepath = Path(output_path)
    filepath.parent.mkdir(parents=True, exist_ok=True)

    with open(filepath, "w", encoding="utf-8") as f:
        for route in routes:
            f.write(f"{route.prefix} {port_map[route.next_hop]}\n")