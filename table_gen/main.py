import argparse
from pathlib import Path

from config import BUILD_DIR, ROUTE_COUNT, GeneratorType
from route_gen import generate_route_table
from core.mrt_reader import MRTFileType


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate routing tables with port mapping."
    )

    parser.add_argument(
        "-t",
        "--type",
        type=str,
        choices=["mrt", "random"],
        default="random",
        help="Generator type: 'mrt' or 'random'",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=BUILD_DIR / "routes.txt",
        help="Path to save generated route table",
    )
    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        default=None,
        help="Input MRT file path (required for MRT generator)",
    )
    parser.add_argument(
        "-c",
        "--count",
        type=int,
        default=ROUTE_COUNT,
        help="Number of routes to generate or sample",
    )
    parser.add_argument(
        "-s",
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducibility",
    )
    parser.add_argument(
        "-f",
        "--mrt-format",
        type=str,
        choices=["text", "binary"],
        default="text",
        help="MRT file format ('text' or 'binary')",
    )

    return parser.parse_args()


def main() -> None:
    args = parse_args()

    gen_type = GeneratorType.MRT if args.type == "mrt" else GeneratorType.RANDOM
    mrt_fmt = (
        MRTFileType.BINARY if args.mrt_format == "binary" else MRTFileType.TEXT
    )

    if gen_type == GeneratorType.MRT and args.input is None:
        raise ValueError("Input file path (--input / -i) is required for MRT generator.")

    generate_route_table(
        generator_type=gen_type,
        output_path=args.output,
        input_path=args.input,
        count=args.count,
        seed=args.seed,
        mrt_file_type=mrt_fmt,
    )


if __name__ == "__main__":
    main()