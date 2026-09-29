from pathlib import Path

from core.mrt_gen import generate_mrt_routes
from core.gen_types import RouteRecord
from core.mrt_reader import MRTFileType
from core.random_gen import generate_random_routes

from config import GeneratorType, ROUTE_COUNT, PORT_MAP_EXTENSION
from port_mapping import map_next_hops_to_ports
from route_saver import save_routes_and_ports


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
            raise ValueError(
                "input_path is required when generator_type is GeneratorType.MRT")
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
    port_map_path = Path(f"{output_path}{PORT_MAP_EXTENSION}")

    save_routes_and_ports(
        routes=routes,
        port_map=port_map,
        routes_output_path=output_path,
        port_map_output_path=port_map_path,
    )
