#pragma once


struct Ray
{
	f32x3 Origin;
	f32x3 Direction;
};

struct Sphere
{
	f32x3 Center;
	f32 Radius;
};

bool RaySphereIntersects(Ray ray, Sphere sphere, f32& t0)
{
	f32x3 L = sphere.Center - ray.Origin;
	f32 tca = dot(L, ray.Direction);
	f32 d2 = dot(L, L) - tca * tca;
	f32 r2 = sphere.Radius * sphere.Radius;
	if (d2 > r2) return false;
	f32 thc = sqrtf(r2 - d2);
	t0 = tca - thc;
	f32 t1 = tca + thc;
	if (t0 < 0) t0 = t1;
	if (t0 < 0) return false;
	return true;
}

f32x3 CastRay(Ray r, Sphere s)
{
	float dst = std::numeric_limits<float>::max();
	if (RaySphereIntersects(r, s, dst))
	{
		return{ 0.4f, 0.4f, 0.3f };
	}
	return{ 0.2f, 0.7f, 0.3f };
}







void ApplicationState::Render()
{
	win32_offscreen_buffer* Buffer = &BackBuffer;
	if (HasRenderedThisFrame) return;

	i32 Width = Buffer->Width;
	i32 Height = Buffer->Height;

	f32* RenderTarget = Buffer->RenderTarget;
	//::Render(RenderTarget, Width, Height, 3);
	//DemofoxRenderScalar(RenderTarget, Width, Height, 3);
	//DemofoxRenderScalarBranchless(RenderTarget, Width, Height, 3);


	i32 NumTilesX = 2;
	i32 NumTilesY = 6;
	i32 TileWidth = Width / NumTilesX;
	i32 TileHeight = Height / NumTilesY;
	DemofoxRenderSimtPooled(RenderTarget, Width, Height, NumTilesX, NumTilesY, TileWidth, TileHeight, 3);
	/*
	Sphere sphere = { {-3.f, 0.f, -16.f }, 4.f};

	f32 tanFov = tan(90 / 2.f); // 1

	f32 y_const_term = - tanFov / (f32)Height + tanFov;
	f32 y_const_factor = -tanFov * 2.f / (f32)Height;

	f32 x_const_term = (tanFov / (f32)Height) * (1.f - Width);
	f32 x_const_factor = 2.f * tanFov / (f32)Height;


	for (i32 Y = 0; Y < Height; ++Y)
	{
		f32 y = Y * y_const_factor + y_const_term;

		for (i32 X = 0; X < Width; ++X)
		{

			f32 x = X * x_const_factor + x_const_term;
			f32x3 udir = { x, y, -1.f };
			f32x3 dir = normalize(udir);
			f32x3 color = CastRay({{ 0.f, 0.f, 0.f }, dir}, sphere);

			RenderTarget[(Y * Width + X) * 3 + 0] = color.x;
			RenderTarget[(Y * Width + X) * 3 + 1] = color.y;
			RenderTarget[(Y * Width + X) * 3 + 2] = color.z;
		}
	}
	*/
	// Wide Copy out
#if 0
	auto clamp = [](f32 x, f32 a, f32 b) { return (x < a) ? a : (x > b ? b : x); };
	u8* Row = (u8*)Buffer->Memory;
	for (u32 Y = 0; Y < Height; ++Y)
	{
		u32* Pixel = (u32*)Row;
		for (u32 X = 0; X < Width / LANE_COUNT; ++X)
		{
			u32 RegisterIndex = (Y * Width + X * LANE_COUNT) * 3;
			for (u32 Z = 0; Z < LANE_COUNT; Z++)
			{
				u32 PixelIndex = RegisterIndex + Z;
				u8 R = (u8)((i32)(clamp(RenderTarget[PixelIndex + 0 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				u8 G = (u8)((i32)(clamp(RenderTarget[PixelIndex + 1 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				u8 B = (u8)((i32)(clamp(RenderTarget[PixelIndex + 2 * LANE_COUNT], 0.f, 1.f) * 255) & 0xFF);
				Pixel[Z] = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
			}

			Pixel += LANE_COUNT;
		}
		Row += Buffer->Pitch;
	}
#elif 1

	auto clamp = [](f32 x, f32 a, f32 b) { return (x < a) ? a : (x > b ? b : x); };
	for (i32 TileX = 0; TileX < NumTilesX; TileX++)
	{
		i32 TileMinX = TileX * TileWidth;
		i32 TileMaxX = TileMinX + (TileWidth - 1);
		for (i32 TileY = 0; TileY < NumTilesY; TileY++)
		{
			i32 TileMinY = TileY * TileHeight;
			i32 TileMaxY = TileMinY + (TileHeight - 1);

			i32 TileSize = TileHeight * TileWidth * 3;
			i32 TileYOffset = TileY * TileHeight * Width * 3;
			i32 TileXOffset = TileSize * TileX;
			i32 BufferTileOffset = TileXOffset + TileYOffset;

			f32* BufferPos = RenderTarget + BufferTileOffset;

			for (i32 Y = TileMinY; Y <= TileMaxY; Y++)
			{
				for (i32 X = TileMinX; X <= TileMaxX; X += LANE_COUNT)
				{
					f32* R = &BufferPos[0];
					f32* G = &BufferPos[1 * LANE_COUNT];
					f32* B = &BufferPos[2 * LANE_COUNT];

					m256x3 FrameColor = m256x3{ load_ps(R), load_ps(G), load_ps(B) };
					u32* Pixel = (u32*)(Buffer->Memory) + ((u64)Y * (u64)Width + (u64)X);
					for (u32 Z = 0; Z < LANE_COUNT; Z++)
					{
						u8 R = (u8)((i32)(clamp(FrameColor.x.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						u8 G = (u8)((i32)(clamp(FrameColor.y.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						u8 B = (u8)((i32)(clamp(FrameColor.z.m256_f32[Z], 0.f, 1.f) * 255) & 0xFF);
						Pixel[Z] = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
					}
					BufferPos += LANE_COUNT * 3;
				}
			}
		}
	}

#else
	// Scalar copy out

	auto clamp = [](f32 x, f32 a, f32 b) { return x < a ? a : (x > b ? b : x); };
	u8* Row = (u8*)Buffer->Memory;
	for (i32 Y = 0; Y < Height; ++Y)
	{
		u32* Pixel = (u32*)Row;
		for (i32 X = 0; X < Width; ++X)
		{
			i32 PixelIndex = (Y * Width + X) * 3;
			u8 R = (u8)((i32)(clamp(RenderTarget[PixelIndex + 0], 0.f, 1.f) * 255) & 0xFF);
			u8 G = (u8)((i32)(clamp(RenderTarget[PixelIndex + 1], 0.f, 1.f) * 255) & 0xFF);
			u8 B = (u8)((i32)(clamp(RenderTarget[PixelIndex + 2], 0.f, 1.f) * 255) & 0xFF);
			*Pixel = (((u32)R) << 16) | (((u32)G) << 8) | (u32)B | 0;
			Pixel += 1;
		}
		Row += Buffer->Pitch;
	}
#endif
	HasRenderedThisFrame = true;
}





//static
//u32 wang_hash(u32& seed)
//{
//    seed = u32(seed ^ u32(61)) ^ u32(seed >> u32(16));
//    seed *= u32(9);
//    seed = seed ^ (seed >> 4);
//    seed *= u32(0x27d4eb2d);
//    seed = seed ^ (seed >> 15);
//    return seed;
//}
//
//static
//f32 Randomf3201(u32& state)
//{
//    return f32(wang_hash(state)) / 4294967296.0f;
//}
//
//static
//m256x3 RandomUnitVector(u32& state)
//{
//    __m256 wide_z = set_ps(Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state));
//    __m256 wide_a = set_ps(Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state), Randomf3201(state));
//    __m256 z = wide_z * set1_ps(2.f) - ConstOne;
//
//    __m256 a = wide_a * set1_ps(c_twopi);
//    __m256 r = sroot(ConstOne - z * z);
//    __m256 x = r * cos_ps(a);
//    __m256 y = r * sin_ps(a);
//    return m256x3{ x, y, z };
//}


//_mm256_srai_epi32
//_mm256_srli_epi32/