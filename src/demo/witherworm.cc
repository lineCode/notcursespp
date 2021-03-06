#include <config.hh>

#include <memory>
#include <vector>
#include <cassert>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include "demo.hh"

#include <ncpp/NotCurses.hh>

using namespace ncpp;

// Fill up the screen with as much crazy Unicode as we can, and then set a
// gremlin loose, looking to brighten up the world.

static std::shared_ptr<Plane>
mathplane (NotCurses &nc)
{
  int dimx, dimy;
  nc.get_term_dim (&dimy, &dimx);

  const int HEIGHT = 9;
  const int WIDTH = dimx;
  auto n = std::make_shared<Plane> (HEIGHT, WIDTH, dimy - HEIGHT - 1, dimx - WIDTH - 1);
  Cell b;

  b.set_bg_alpha (Cell::AlphaTransparent);
  b.set_fg_alpha (Cell::AlphaTransparent);
  n->set_base (b);
  n->release (b);

  if (n) {
  /* FIXME issue #260
    struct ncplane* stdn = notcurses_stdplane(nc);
    ncplane_set_bg_alpha(n, CELL_ALPHA_TRANSPARENT);
    int snatchy = dimy - HEIGHT - 1;
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);
    // FIXME reenable the left parts of these strings, issue #260*/
    //ncplane_printf_aligned(n, 0, NCALIGN_RIGHT, /*∮E⋅da=Q,n→∞,∑f(i)=∏g(i)*/"⎧⎡⎛┌─────┐⎞⎤⎫");
    /*ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);
    ncplane_printf_aligned(n, 1, NCALIGN_RIGHT, "⎪⎢⎜│a²+b³ ⎟⎥⎪");
    ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);*/
    //ncplane_printf_aligned(n, 2, NCALIGN_RIGHT, /*∀x∈ℝ:⌈x⌉=−⌊−x⌋,α∧¬β=¬(¬α∨β)*/"⎪⎢⎜│───── ⎟⎥⎪");
    /*ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "⎪⎢⎜⎷ c₈   ⎟⎥⎪");
    ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);*/
    //ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, /*ℕ⊆ℕ₀⊂ℤ⊂ℚ⊂ℝ⊂ℂ(z̄=ℜ(z)−ℑ(z)⋅𝑖)*/"⎨⎢⎜       ⎟⎥⎬");
    /*ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);
    ncplane_printf_aligned(n, 5, NCALIGN_RIGHT, "⎪⎢⎜ ∞     ⎟⎥⎪");
    ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);*/
    //ncplane_printf_aligned(n, 6, NCALIGN_RIGHT, /*⊥<a≠b≡c≤d≪⊤⇒(⟦A⟧⇔⟪B⟫)*/"⎪⎢⎜ ⎲     ⎟⎥⎪");
    /*ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);
    ncplane_printf_aligned(n, 7, NCALIGN_RIGHT, "⎪⎢⎜ ⎳aⁱ-bⁱ⎟⎥⎪");
    ncplane_at_yx(stdn, snatchy++, 0, &c);
    ncplane_set_fg(n, cell_fchannel(&c) & CELL_BG_MASK);*/
    //ncplane_printf_aligned(n, 8, NCALIGN_RIGHT, /*2H₂+O₂⇌2H₂O,R=4.7kΩ,⌀200µm*/"⎩⎣⎝i=1    ⎠⎦⎭");
  }

  return n;
}

// get the (up to) eight surrounding cells. they run clockwise, starting from
// the upper left: 012
//                 7 3
//                 654
// is the provided cell part of the wall (i.e. a box-drawing character)?
static bool
wall_p (const Plane *n, const Cell &c)
{
	if (c.is_simple ()) { // any simple cell is fine to consume
		return false;
	}

	const char* egc = n->get_extended_gcluster (c);
	wchar_t w;
	if (mbtowc (&w, egc, strlen(egc)) > 0) {
		if (w >= 0x2500 && w <= 0x257f) { // no room in the inn, little worm!
			return true;
		}
	}

	return false;
}

// the closer the coordinate is (lower distance), the more we lighten the cell
static inline int
lighten (Plane *n, Cell &c, int distance, int y, int x)
{
	if (c.is_wide_right ()) { // not really a character
		return 0;
	}

	unsigned r, g, b;
	c.get_fg_rgb (&r, &g, &b);
	r += rand () % ((r + 16) / (4 * distance + 1) + 1);
	g += rand () % ((g + 16) / (4 * distance + 1) + 1);
	b += rand () % ((b + 16) / (4 * distance + 1) + 1);

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (!c.set_fg_rgb (r, g, b)) {
		return -1;
	}

	return n->putc (y, x, c);
}

static void
surrounding_cells (Plane *n, Cell *cells, int y, int x)
{
	n->get_at (y - 1, x - 1, cells[0]);
	n->get_at (y - 1, x, cells[1]);
	n->get_at (y - 1, x + 1, cells[2]);
	n->get_at (y, x + 1, cells[3]);
	n->get_at (y + 1, x + 1, cells[4]);
	n->get_at (y + 1, x, cells[5]);
	n->get_at (y + 1, x - 1, cells[6]);
	n->get_at (y, x - 1, cells[7]);
	n->get_at (y - 2, x, cells[8]);
	n->get_at (y + 2, x, cells[9]);
	n->get_at (y, x - 2, cells[10]);
	n->get_at (y, x + 2, cells[11]);
	n->get_at (y, x, cells[12]);
}

static bool
lightup_surrounding_cells (Plane *n, const Cell cells[], int y, int x)
{
	Cell c;
	n->duplicate (c, cells[0]);
	lighten (n, c, 2, y - 1, x - 1);

	n->duplicate (c, cells[1]);
	lighten (n, c, 1, y - 1, x);

	n->duplicate (c, cells[2]);
	lighten (n, c, 2, y - 1, x + 1);

	n->duplicate (c, cells[7]);
	lighten (n, c, 1, y, x - 1);

	n->duplicate (c, cells[3]);
	lighten (n, c, 1, y, x + 1);

	n->duplicate (c, cells[6]);
	lighten (n, c, 2, y + 1, x - 1);


	n->duplicate (c, cells[5]);
	lighten (n, c, 1, y + 1, x);

	n->duplicate (c, cells[4]);
	lighten (n, c, 2, y + 1, x + 1);

	n->duplicate (c, cells[8]);
	lighten (n, c, 2, y - 2, x);

	n->duplicate (c, cells[9]);
	lighten (n, c, 2, y + 2, x);

	n->duplicate (c, cells[10]);
	lighten (n, c, 2, y, x - 2);

	n->duplicate (c, cells[11]);
	lighten (n, c, 2, y, x + 2);

	n->duplicate (c, cells[12]);
	lighten (n, c, 0, y, x);

	n->release (c);
	return true;
}

typedef struct worm {
	Cell lightup[13];
	int x, y;
	int prevx, prevy;
} worm;

static void
init_worm (worm* s, int dimy, int dimx)
{
	// start them in the lower 3/4 of the screen
	s->y = random () % dimy;
	s->x = random () % dimx;
	s->prevx = 0;
	s->prevy = 0;
}

static bool
wormy_top (NotCurses &nc, worm* s)
{
	Plane *n = nc.get_stdplane ();
	surrounding_cells (n, s->lightup, s->y, s->x);
	if (!lightup_surrounding_cells (n, s->lightup, s->y, s->x)) {
		return false;
	}
	return true;
}

static bool
wormy (NotCurses &nc, worm* s, int dimy, int dimx)
{
	Plane *n = nc.get_stdplane ();
	int oldy, oldx;
	Cell c;

	do { // force a move
		oldy = s->y;
		oldx = s->x;
		// FIXME he ought be weighted to avoid light; he's a worm after all
		int direction = random () % 4;
		switch (direction) {
			case 0: --s->y; break;
			case 1: ++s->x; break;
			case 2: ++s->y; break;
			case 3: --s->x; break;
		}

		// keep him away from the sides due to width irregularities
		if (s->y <= 0) {
			s->y = dimy - 1;
		}

		if(s->y >= dimy) {
			s->y = 0;
		}

		if (s->x <= 0){
			s->x = dimx - 1;
		}
		if (s->x >= dimx) {
			s->x = 0;
		}

		n->get_at (s->y, s->x, &c);
		// don't allow the worm into the summary zone (test for walls)
		if (wall_p (n, c)) {
			s->x = oldx;
			s->y = oldy;
		}
	} while ((oldx == s->x && oldy == s->y) || (s->x == s->prevx && s->y == s->prevy));
	s->prevy = oldy;
	s->prevx = oldx;
	n->release (c);

	return true;
}

#include <cxxabi.h>

using namespace __cxxabiv1;

char* util_demangle(std::string to_demangle)
{
    int status = 0;
    return __cxxabiv1::__cxa_demangle(to_demangle.c_str(), NULL, NULL, &status);
}

// each worm wanders around aimlessly, prohibited from entering the summary
// section. it ought light up the cells around it; to do this, we keep an array
// of 13 cells with the original colors, which we tune up for the duration of
// our colocality (unless they're summary area walls).
static void *
worm_thread ([[maybe_unused]] void* vnc)
{
	NotCurses &nc = NotCurses::get_instance ();
	Plane *n = nc.get_stdplane ();
	int dimy, dimx;
	n->get_dim (&dimy, &dimx);

	int wormcount = (dimy * dimx) / 800;
	std::vector<worm> worms (wormcount);
	for (int s = 0 ; s < wormcount ; ++s) {
		init_worm (&worms[s], dimy, dimx);
	}

	struct timespec iterdelay = { /*.tv_sec =*/ 0, /*.tv_nsec =*/ 100000000ul / 20, };
	while (true) {
		pthread_testcancel ();
		for (int s = 0 ; s < wormcount ; ++s) {
			if (!wormy_top (nc, &worms[s])) {
				return nullptr;
			}
		}

		if (!demo_render (nc)) {
			return nullptr;
		}

		for (int s = 0 ; s < wormcount ; ++s) {
			if (!wormy (nc, &worms[s], dimy, dimx)) {
				return nullptr;
			}
		}

		nanosleep (&iterdelay, nullptr);
	}

	return nullptr;
}

static bool
message (std::shared_ptr<Plane> n, int maxy, int maxx, int num, int total, int bytes_out, int egs_out, int cols_out)
{
	Cell c;
	c.set_fg_alpha (Cell::AlphaTransparent);
	c.set_bg_alpha (Cell::AlphaTransparent);
	n->set_base (c);
	n->release (c);

	n->set_fg_rgb (255, 255, 255);
	n->set_bg_rgb (32, 64, 32);

	uint64_t channels = 0;
	channels_set_fg_rgb (&channels, 255, 255, 255);
	channels_set_bg_rgb (&channels, 32, 64, 32);
	n->cursor_move (2, 0);
	if (!n->rounded_box (0, channels, 4, 56, 0)) {
		return false;
	}

	// bottom handle
	n->putc (4, 17, "┬", nullptr);
	n->putc (5, 17, "│", nullptr);
	n->putc (6, 17, "╰", nullptr);

	Cell hl;
	n->load (hl, "─");
	hl.set_fg_rgb (255, 255, 255);
	hl.set_bg_rgb (32, 64, 32);
	n->hline (hl, 57 - 18 - 1);

	n->putc (6, 56, "╯", nullptr);
	n->putc (5, 56, "│", nullptr);
	n->putc (4, 56, "┤", nullptr);

	// top handle
	n->putc (2, 3, "╨", nullptr);
	n->putc (1, 3, "║", nullptr);
	n->putc (0, 3, "╔", nullptr);
	n->load (hl, "═");
	n->hline (hl, 20 - 4 - 1);
	n->release (hl);

	n->putc (0, 19, "╗", nullptr);
	n->putc (1, 19, "║", nullptr);
	n->putc (2, 19, "╨", nullptr);
	n->set_fg_rgb (64, 128, 240);
	n->set_bg_rgb (32, 64, 32);
	n->styles_on (CellStyle::Italic);
	n->printf (5, 18, " bytes: %05d EGCs: %05d cols: %05d ", bytes_out, egs_out, cols_out);
	n->printf (1, 4, " %03dx%03d (%d/%d) ", maxx, maxy, num + 1, total);
	n->styles_off (CellStyle::Italic);
	n->set_fg_rgb (224, 128, 224);
	n->putstr (3, 1, "  🔥 unicode 13, resize awareness, 24b directcolor…🔥  ");
	n->set_fg_rgb (255, 255, 255);

	return true;
}

// Much of this text comes from http://kermitproject.org/utf8.html
bool witherworm_demo (NotCurses &nc)
{
	static const char* strs[] = {
		"Война и мир",
		"Бра́тья Карама́зовы",
		"Час сэканд-хэнд",
		"Tonio Kröger",
		"Meg tudom enni az üveget, nem lesztőle bajom",
		"Voin syödä lasia, se ei vahingoita minua",
		"Sáhtán borrat lása, dat ii leat bávččas",
		"Мон ярсан суликадо, ды зыян эйстэнзэ а ули",
		"Mie voin syvvä lasie ta minla ei ole kipie",
		"Minä voin syvvä st'oklua dai minule ei ole kibie",
		"Ma võin klaasi süüa, see ei tee mulle midagi",
		"Es varu ēst stiklu, tas man nekaitē",
		"Aš galiu valgyti stiklą ir jis manęs nežeidži",
		"Mohu jíst sklo, neublíží mi",
		"Môžem jesť sklo. Nezraní ma",
		"Mogę jeść szkło i mi nie szkodzi",
		"Lahko jem steklo, ne da bi mi škodovalo",
		"Ja mogu jesti staklo, i to mi ne šteti",
		"Ја могу јести стакло, и то ми не штети",
		"Можам да јадам стакло, а не ме штета",
		"Я могу есть стекло, оно мне не вредит",
		"Я магу есці шкло, яно мне не шкодзіць",
		"Osudy dobrého vojáka Švejka za světové války",
		"kācaṃ śaknomyattum; nopahinasti mām",
		"ὕαλον ϕαγεῖν δύναμαι· τοῦτο οὔ με βλάπτει",
		"Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα",
		"Vitrum edere possum; mihi non nocet",
		"iℏ∂∂tΨ=−ℏ²2m∇2Ψ+VΨ",
		"Je puis mangier del voirre. Ne me nuit",
		"Je peux manger du verre, ça ne me fait pas mal",
		"Pòdi manjar de veire, me nafrariá pas",
		"J'peux manger d'la vitre, ça m'fa pas mal",
		"Dji pou magnî do vêre, çoula m' freut nén må",
		"Ch'peux mingi du verre, cha m'foé mie n'ma",
		"F·ds=ΔE",
		"Mwen kap manje vè, li pa blese'm",
		"Kristala jan dezaket, ez dit minik ematen",
		"Puc menjar vidre, que no em fa mal",
		"overall there is a smell of fried onions",
		"Puedo comer vidrio, no me hace daño",
		"Puedo minchar beire, no me'n fa mal",
		"Eu podo xantar cristais e non cortarme",
		"Posso comer vidro, não me faz mal",
		"Posso comer vidro, não me machuca",
		"ஸீரோ டிகிரி",
		"بين القصرين",
		"قصر الشوق",
		"السكرية",
		"三体",
		"血的神话公元年湖南道县文革大屠杀纪实",
		"三国演义",
		"紅樓夢",
		"Hónglóumèng",
		"红楼梦",
		"महाभारतम्",
		"Mahābhāratam",
		" रामायणम्",
		"Rāmāyaṇam",
		"القرآن",
		"תּוֹרָה",
		"תָּנָ״ךְ",
		"Σίβνλλα τί ϴέλεις; respondebat illa: άπο ϴανεΐν ϴέλω",
		"① На всей земле был один язык и одно наречие.",
		"② А кад отидоше од истока, нађоше равницу у земљи сенарској, и населише се онде.",
		"③ І сказалі адно аднаму: наробім цэглы і абпалім агнём. І стала ў іх цэгла замест камянёў, а земляная смала замест вапны.",
		"④ І сказали вони: Тож місто збудуймо собі, та башту, а вершина її аж до неба. І вчинімо для себе ймення, щоб ми не розпорошилися по поверхні всієї землі.",
		"A boy has never wept nor dashed a thousand kim",
		"⑤ Господ слезе да ги види градот и кулата, што луѓето ги градеа.",
		"⑥ И҆ речѐ гдⷭ҇ь: сѐ, ро́дъ є҆ди́нъ, и҆ ѹ҆стнѣ̀ є҆ди҄нѣ всѣ́хъ, и҆ сїѐ нача́ша твори́ти: и҆ нн҃ѣ не ѡ҆скꙋдѣ́ютъ ѿ ни́хъ всѧ҄, є҆ли҄ка а́҆ще восхотѧ́тъ твори́ти.",
		"⑦ Ⱂⱃⰻⰻⰴⱑⱅⰵ ⰺ ⰺⰸⱎⰵⰴⱎⰵ ⱄⰿⱑⱄⰻⰿⱏ ⰺⰿⱏ ⱅⱆ ⱔⰸⱏⰹⰽⰻ ⰺⱈⱏ · ⰴⰰ ⱀⰵ ⱆⱄⰾⱏⰹⱎⰰⱅⱏ ⰽⱁⰶⰴⱁ ⰴⱃⱆⰳⰰ ⱄⰲⱁⰵⰳⱁ ⁖⸏",
		"काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम्",
		"色は匂へど 散りぬるを 我が世誰ぞ 常ならむ 有為の奥山 今日越えて 浅き夢見じ 酔ひもせず",
		"いろはにほへど　ちりぬるを わがよたれぞ　つねならむ うゐのおくやま　けふこえて あさきゆめみじ　ゑひもせず",
		"मलाई थाहा छैन । म यहाँ काम मात्र गर्छु ",
		"ብርሃነ ዘርኣይ",
		"ኃይሌ ገብረሥላሴ",
		"ᓱᒻᒪᓂᒃᑯᐊ ᐃᓄᑦᑎᑐᑐᐃᓐᓇᔭᙱᓚᑦ",
		"∮ E⋅da = Q,  n → ∞, ∑ f(i) = ∏ g(i), ∀x∈ℝ: ⌈x⌉ = −⌊−x⌋, α ∧ ¬β = ¬(¬α ∨ β)"
		"2H₂ + O₂ ⇌ 2H₂O, R = 4.7 kΩ, ⌀ 200mm",
		"ði ıntəˈnæʃənəl fəˈnɛtık əsoʊsiˈeıʃn",
		"((V⍳V)=⍳⍴V)/V←,V    ⌷←⍳→⍴∆∇⊃‾⍎⍕⌈",
		"Eڿᛯℇ✈ಅΐʐ𝍇Щঅ℻ ⌬⌨ ⌣₰ ⠝ ‱ ‽ ח ֆ ∜ ⨀ ĲႪ ⇠ ਐ ῼ இ ╁ ଠ ୭ ⅙ ㈣⧒ ₔ ⅷ ﭗ ゛〃・ ↂ ﻩ ✞ ℼ ⌧",
		"M' podê cumê vidru, ca ta maguâ-m'",
		"Ami por kome glas anto e no ta hasimi daño",
		"六四事件八九民运动态网自由门天安门天安门法轮功李洪志六四天安门事件天安门大屠杀反右派斗争大跃进政策文化大革命人权民运自由独立I多党制台湾台湾T中华民国西藏土伯特唐古特达赖喇嘛法轮功新疆维吾尔自治区诺贝尔和平奖刘暁波民主言论思想反共反革命抗议运动骚乱暴乱骚扰扰乱抗暴平反维权示威游行李洪志法轮大法大法弟子强制断种强制堕胎民族净化人体实验肃清胡耀邦赵紫阳魏京生王丹还政于民和平演变激流中国北京之春大纪元时报评论共产党独裁专制压制统监视镇压迫害 侵略掠夺破坏拷问屠杀活摘器官诱拐买卖人口游进走私毒品卖淫春画赌博六合彩天安门天安门法轮功李洪志刘晓波动态网自由门",
		"Posso mangiare il vetro e non mi fa male",
		"زَّ وَجَلَّ فَمَا وَجَدْنَا فِيهِ مِنْ حَلاَلٍ اسْتَحْلَلْنَاهُ وَمَا وَجَدْنَا فِيهِ مِنْ حَرَامٍ حَرَّمْنَاهُ . أَلاَ وَإِنَّ مَا حَرَّمَ رَسُولُ اللَّهِ ـ صلى الله عليه وسلم ـ مِثْلُ مَا حَرَّمَ اللَّ",
		"śrī-bhagavān uvāca kālo 'smi loka-kṣaya-kṛt pravṛddho lokān samāhartum iha pravṛttaḥ ṛte 'pi tvāṁ na bhaviṣyanti sarve ye 'vasthitāḥ pratyanīkeṣu yodhāḥ",
		"الحرام لذاتهالحرام لغيره",
		"Je suis Charli",
		"Sôn bôn de magnà el véder, el me fa minga mal",
		"Ewige Blumenkraft",
		"HEUTE DIE WELT MORGENS DAS SONNENSYSTEM",
		"Me posso magna' er vetro, e nun me fa male",
		"M' pozz magna' o'vetr, e nun m' fa mal",
		"μῆλον τῆς Ἔριδος",
		"verwirrung zweitracht unordnung beamtenherrschaft grummet",
		"Mi posso magnare el vetro, no'l me fa mae",
		"Pòsso mangiâ o veddro e o no me fà mâ",
		"Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn",
		"ineluctable modality of the visible",
		"Une oasis d'horreur dans un désert d'ennui",
		"E pur si muov",
		"Lasciate ogne speranza, voi ch'intrate",
		"∀u1…∀uk[∀x∃!yφ(x,y,û) → ∀w∃v∀r(r∈v ≡ ∃s(s∈w & φx,y,û[s,r,û]))]",
		"Puotsu mangiari u vitru, nun mi fa mali",
		"Jau sai mangiar vaider, senza che quai fa donn a mai",
		"Pot să mănânc sticlă și ea nu mă rănește",
		"‽⅏⅋℺℧℣",
		"Mi povas manĝi vitron, ĝi ne damaĝas min",
		"Mý a yl dybry gwéder hag éf ny wra ow ankenya",
		"Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi",
		"Foddym gee glonney agh cha jean eh gortaghey mee",
		"᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ",
		"Con·iccim ithi nglano. Ním·géna",
		"⚔☢☭࿗☮࿘☭☣",
		"Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom",
		"Ithim-sa gloine agus ní miste damh é",
		"S urrainn dhomh gloinne ithe; cha ghoirtich i mi",
		"ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ",
		"Ic mæg glæs eotan ond hit ne hearmiað me",
		"Ich canne glas eten and hit hirtiþ me nouȝt",
		"I can eat glass and it doesn't hurt me",
		"aɪ kæn iːt glɑːs ænd ɪt dɐz nɒt hɜːt mi",
		"⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍",
		"Mi kian niam glas han i neba hot mi",
		"Ah can eat gless, it disnae hurt us",
		"𐌼𐌰𐌲 𐌲𐌻𐌴𐍃 𐌹̈𐍄𐌰𐌽, 𐌽𐌹 𐌼𐌹𐍃 𐍅𐌿 𐌽𐌳𐌰𐌽 𐌱𐍂𐌹𐌲𐌲𐌹𐌸",
		"ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨ",
		"Ek get etið gler án þess að verða sár",
		"Eg kan eta glas utan å skada meg",
		"Jeg kan spise glass uten å skade meg",
		"Eg kann eta glas, skaðaleysur",
		"Ég get etið gler án þess að meiða mig",
		"𝐸 = 𝑚𝑐²",
		"Jag kan äta glas utan att skada mig",
		"Jeg kan spise glas, det gør ikke ondt på mig",
		"㎚ ㎛ ㎜ ㎝ ㎞ ㎟ ㎠ ㎡ ㎢ ㎣ ㎤ ㎥ ㎦ ㎕ ㎖ ㎗ ㎘ ㏄ ㎰ ㎱ ㎲ ㎳ ㎍ ㎎ ㎏ ㎅ ㎆ ㏔ ㎇ ㎐ ㎑ ㎒ ㎓ ㎔㎮ ㎯",
		"Æ ka æe glass uhen at det go mæ naue",
		"က္ယ္ဝန္တော္၊က္ယ္ဝန္မ မ္ယက္စားနုိင္သည္။ ၎က္ရောင္ ထိခုိက္မ္ဟု မရ္ဟိပာ။",
		"ကျွန်တော် ကျွန်မ မှန်စားနိုင်တယ်။ ၎င်းကြောင့် ထိခိုက်မှုမရှိပါ။ ",
		"Tôi có thể ăn thủy tinh mà không hại gì",
		"些 𣎏 世 咹 水 晶 𦓡 空 𣎏 害",
		"ខ្ញុំអាចញុំកញ្ចក់បាន ដោយគ្មានបញ្ហា",
		"ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ",
		"ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็",
		"Би шил идэй чадна, надад хортой би",
		"ᠪᠢ ᠰᠢᠯᠢ ᠢᠳᠡᠶᠦ ᠴᠢᠳᠠᠨᠠ ᠂ ᠨᠠᠳᠤᠷ ᠬᠣᠤᠷᠠᠳᠠᠢ ᠪᠢᠰ",
		"म काँच खान सक्छू र मलाई केहि नी हुन्न्",
		"ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད",
		"我能吞下玻璃而不伤身体",
		"我能吞下玻璃而不傷身體",
		"Góa ē-tàng chia̍h po-lê, mā bē tio̍h-siong",
		"私はガラスを食べられますそれは私を傷つけません",
		"나는 유리를 먹을 수 있어요. 그래도 아프지 않아",
		"Mi save kakae glas, hemi no save katem mi",
		"Hiki iaʻu ke ʻai i ke aniani; ʻaʻole nō lā au e ʻeha",
		"E koʻana e kai i te karahi, mea ʻā, ʻaʻe hauhau",
		"ᐊᓕᒍᖅ ᓂᕆᔭᕌᖓᒃᑯ ᓱᕋᙱᑦᑐᓐᓇᖅᑐ",
		"Naika məkmək kakshət labutay, pi weyk ukuk munk-sik nay",
		"Tsésǫʼ yishą́ągo bííníshghah dóó doo shił neezgai da",
		"mi kakne le nu citka le blaci .iku'i le se go'i na xrani m",
		"Ljœr ye caudran créneþ ý jor cẃran",
		"Ik kin glês ite, it docht me net sear",
		"Ik kan glas eten, het doet mĳ geen kwaad",
		"Iech ken glaas èèse, mer 't deet miech jing pieng",
		"Ek kan glas eet, maar dit doen my nie skade nie",
		"Ech kan Glas iessen, daat deet mir nët wei",
		"Ich kann Glas essen, ohne mir zu schaden",
		"Ich kann Glas verkasematuckeln, ohne dattet mich wat jucken tut",
		"Isch kann Jlaas kimmeln, uuhne datt mich datt weh dääd",
		"Ich koann Gloos assn und doas dudd merr ni wii",
		"Мен шиша ейишим мумкин, аммо у менга зарар келтирмайди",
		"আমি কাঁচ খেতে পারি, তাতে আমার কোনো ক্ষতি হয় না",
		"मी काच खाऊ शकतो, मला ते दुखत नाही",
		"ನನಗೆ ಹಾನಿ ಆಗದೆ, ನಾನು ಗಜನ್ನು ತಿನಬಹು",
		"मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती",
		"എനിക്ക് ഗ്ലാസ് തിന്നാം. അതെന്നെ വേദനിപ്പിക്കില്ല",
		"நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது",
		"నేను గాజు తినగలను మరియు అలా చేసినా నాకు ఏమి ఇబ్బంది లే",
		"මට වීදුරු කෑමට හැකියි. එයින් මට කිසි හානියක් සිදු නොවේ",
		"میں کانچ کھا سکتا ہوں اور مجھے تکلیف نہیں ہوتی",
		"زه شيشه خوړلې شم، هغه ما نه خوږو",
		".من می توانم بدونِ احساس درد شيشه بخور",
		"أنا قادر على أكل الزجاج و هذا لا يؤلمني",
		"Nista' niekol il-ħġieġ u ma jagħmilli xejn",
		"אני יכול לאכול זכוכית וזה לא מזיק לי",
		"איך קען עסן גלאָז און עס טוט מיר נישט װײ",
		"Metumi awe tumpan, ɜnyɜ me hwee",
		"Iech konn glaasch voschbachteln ohne dass es mir ebbs daun doun dud",
		"'sch kann Glos essn, ohne dass'sch mer wehtue",
		"Isch konn Glass fresse ohne dasses mer ebbes ausmache dud",
		"I kå Glas frässa, ond des macht mr nix",
		"I ka glas eassa, ohne dass mar weh tuat",
		"I koh Glos esa, und es duard ma ned wei",
		"卂丂爪𝑜Ⓓє𝓊Ⓢ в𝔢𝓁Įαlнᗩs𝓉𝐮ℝ 𝐍чคⓇ𝓛Ａт卄ＯｔＥᵖ 𝔴ᗝⓣ𝓐ⓝ 𝐧ίｇ𝕘𝐔尺𝓪ᵗ𝕙 𝔻ĤＯ𝔩ᵉ𝔰 卂žᵃ𝓣ĤỖ𝔱𝓗 Ť𝔦ℕ𝔻Ａℓ๏Ş ᛕＡĐ𝐈𝓽ħ",
		"I kaun Gloos essen, es tuat ma ned weh",
		"Ich chan Glaas ässe, das schadt mir nöd",
		"Ech cha Glâs ässe, das schadt mer ned",
		"Ja mahu jeści škło, jano mne ne škodzić",
		"Я можу їсти скло, і воно мені не зашкодить",
		"Мога да ям стъкло, то не ми вреди",
		"მინას ვჭამ და არა მტკივა",
		"Կրնամ ապակի ուտել և ինծի անհանգիստ չըներ",
		"Unë mund të ha qelq dhe nuk më gjen gjë",
		"Cam yiyebilirim, bana zararı dokunmaz",
		"جام ييه بلورم بڭا ضررى طوقونم",
		"Алам да бар, пыяла, әмма бу ранит мине",
		"Men shisha yeyishim mumkin, ammo u menga zarar keltirmaydi",
		"Inā iya taunar gilāshi kuma in gamā lāfiyā",
		"إِنا إِىَ تَونَر غِلَاشِ كُمَ إِن غَمَا لَافِىَ",
		"Mo lè je̩ dígí, kò ní pa mí lára",
		"Nakokí kolíya biténi bya milungi, ekosála ngáí mabé tɛ́",
		"Naweza kula bilauri na sikunyui",
		"Saya boleh makan kaca dan ia tidak mencederakan saya",
		"Kaya kong kumain nang bubog at hindi ako masaktan",
		"Siña yo' chumocho krestat, ti ha na'lalamen yo'",
		"Au rawa ni kana iloilo, ia au sega ni vakacacani kina",
		"Aku isa mangan beling tanpa lara",
		"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗᛋᚳᛖᚪᛚ᛫ᚦᛖᚪᚻ᛫ᛗᚪᚾᚾᚪ᛫ᚷᛖᚻᚹᛦᛚᚳ᛫ᛗᛁᚳᛚᚢᚾ᛫ᚻᛦᛏ᛫ᛞᚫᛚᚪᚾᚷᛁᚠ᛫ᚻᛖ᛫ᚹᛁᛚᛖ᛫ᚠᚩᚱ᛫ᛞᚱᛁᚻᛏᚾᛖ᛫ᛞᚩᛗᛖᛋ᛫ᚻᛚᛇᛏᚪᚾ᛬",
		"An preost wes on leoden, Laȝamon was ihoten He wes Leovenaðes sone -- liðe him be Drihten. He wonede at Ernleȝe at æðelen are chirechen, Uppen Sevarne staþe, sel þar him þuhte, Onfest Radestone, þer he bock radde.",
		"Sîne klâwen durh die wolken sint geslagen, er stîget ûf mit grôzer kraft, ich sih in grâwen tägelîch als er wil tagen, den tac, der im geselleschaft erwenden wil, dem werden man, den ich mit sorgen în verliez. ich bringe in hinnen, ob ich kan. sîn vil manegiu tugent michz leisten hiez.",
		"Τη γλώσσα μου έδωσαν ελληνική το σπίτι φτωχικό στις αμμουδιές του Ομήρου. Μονάχη έγνοια η γλώσσα μου στις αμμουδιές του Ομήρου. από το Άξιον Εστί του Οδυσσέα Ελύτη",
		"На берегу пустынных волн Стоял он, дум великих полн, И вдаль глядел. Пред ним широко Река неслася; бедный чёлн По ней стремился одиноко. По мшистым, топким берегам Чернели избы здесь и там, Приют убогого чухонца; И лес, неведомый лучам В тумане спрятанного солнца, Кругом шумел.",
		"ვეპხის ტყაოსანი შოთა რუსთაველი ღმერთსი შემვედრე, ნუთუ კვლა დამხსნას სოფლისა შრომასა, ცეცხლს, წყალსა და მიწასა, ჰაერთა თანა მრომასა; მომცნეს ფრთენი და აღვფრინდე, მივჰხვდე მას ჩემსა ნდომასა, დღისით და ღამით ვჰხედვიდე მზისა ელვათა კრთომაასა",
		"யாமறிந்த மொழிகளிலே தமிழ்மொழி போல் இனிதாவது எங்கும் காணோம், பாமரராய் விலங்குகளாய், உலகனைத்தும் இகழ்ச்சிசொலப் பான்மை கெட்டு, நாமமது தமிழரெனக் கொண்டு இங்கு வாழ்ந்திடுதல் நன்றோ? சொல்லீர்! தேமதுரத் தமிழோசை உலகமெலாம் பரவும்வகை செய்தல் வேண்டும்.",
		NULL
	};
	const char** s;
	const int steps[] = { 0x10040, 0x100, 0x100, 0x10001, };
	const int starts[] = { 0x10101, 0x004000, 0x000040, 0x400040, };

	Plane *n = nc.get_stdplane ();
	size_t i;
	const size_t screens = sizeof(steps) / sizeof(*steps);

	n->erase ();
	for (i = 0 ; i < screens ; ++i) {
		wchar_t key = NCKEY_INVALID;
		Cell c;
		struct timespec screenend;
		clock_gettime (CLOCK_MONOTONIC, &screenend);
		ns_to_timespec (timespec_to_ns (&screenend) + 5 * timespec_to_ns (&demodelay), &screenend);

		do { // (re)draw a screen
			const int start = starts[i];
			int step = steps[i];
			c.init ();

			int y, x, maxy, maxx;
			n->get_dim (&maxy, &maxx);

			int rgb = start;
			if (!n->cursor_move (0, 0)) {
				return false;
			}

			int bytes_out = 0;
			int egcs_out = 0;
			int cols_out = 0;
			y = 0;
			x = 0;
			n->set_bg_rgb (20, 20, 20);
			do { // we fill up the entire screen, however large, walking our strtable
				s = strs;
				n->set_bg_rgb (20, 20, 20);
				for (s = strs ; *s ; ++s) {
					size_t idx = 0;
					n->get_cursor_yx (&y, &x);
					// fprintf(stderr, "%02d %s\n", y, *s);
					while ((*s)[idx]) { // each multibyte char of string
						if (!n->set_fg_rgb (channel_r(rgb), channel_g(rgb), channel_b(rgb))) {
							return false;
						}

						if (y >= maxy || x >= maxx) {
							break;
						}

						wchar_t wcs;
						int eaten = mbtowc (&wcs, &(*s)[idx], MB_CUR_MAX + 1);
						if (eaten < 0) {
							return false;
						}

						if (iswspace(wcs)) {
							idx += eaten;
							continue;
						}

						int ulen = 0;
						int r;
						if (wcwidth(wcs) <= maxx - x) {
							if ((r = n->putc (&(*s)[idx], &ulen)) < 0) {
								if (ulen < 0) {
									return false;
								}
							}
						} else {
							if ((r = n->putc ('#')) < 1) {
								return false;
							}
						}

						n->get_cursor_yx (&y, &x);
						idx += ulen;
						bytes_out += ulen;
						cols_out += r;
						++egcs_out;
					}
					rgb += step;
				}
			} while (y < maxy && x < maxx);

			std::shared_ptr<Plane> math = mathplane (nc);
			if (!math) {
				return false;
			}

			auto mess = std::make_shared<Plane> (7, 57, 1, 4);
			if (!message (mess, maxy, maxx, i, sizeof(steps) / sizeof(*steps), bytes_out, egcs_out, cols_out)) {
				return false;
			}

			if (!demo_render (nc)) {
				return false;
			}

			if (i) {
				uint64_t delay = timespec_to_ns (&demodelay);
				delay /= screens;
				struct timespec tv;
				if (delay > GIG) {
					ns_to_timespec (GIG, &tv);
				} else {
					ns_to_timespec (delay, &tv);
				}
				n->fadein (&tv, demo_fader);
			}

			pthread_t tid;
			pthread_create (&tid, nullptr, worm_thread, &nc);
			do {
				struct timespec left, cur;
				clock_gettime (CLOCK_MONOTONIC, &cur);
				timespec_subtract (&left, &screenend, &cur);
				key = demo_getc (&left, nullptr);
				clock_gettime (CLOCK_MONOTONIC, &cur);
				int64_t ns = timespec_subtract_ns (&cur, &screenend);
				if (ns > 0) {
					break;
				}
			} while (key < 0);
			pthread_cancel (tid);
			pthread_join (tid, nullptr);
			mess.reset ();

			if (key == NCKEY_RESIZE) {
				nc.resize (&maxy, &maxx);
			}
		} while (key == NCKEY_RESIZE);
	}

	return true;
}
