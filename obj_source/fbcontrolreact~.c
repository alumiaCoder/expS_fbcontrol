#include <ext.h>
#include <ext_obex.h>
#include <math.h>

//ALTERAR ISTO PARA AS FLAGS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define FREQSINSIZE 1500 //número de freqs que estão a entrar pelo sigmund
#define FCENTRALSIZE 5000 //numero de notas no fcentral. Notas defindas pela estrutura freqs. É sempre necessário pelo menos o dobro do que está a ser usado

#define FRAMESTODELETE 5//numero de frames que tem de estar inativa para que possa ser eliminada

struct flags_total { //estrutura que contém as flags das frequEncias e variáveis controlados pelo utilizador
	
	unsigned int frames_to_active;//numero de frames que uma frequencia tem de existir para ser considerada ativa
	unsigned int n_ampR_read; //numero de frames lidos para calculo da taxa de variação de amplitude
	
	float ampmax;
	float ampmin;

	int nota_proibidaE;
	int nota_proibidaE2;

	int nota_proibidaR;
	int nota_proibidaR2;

	int algoOn; //liga ou desliga o algoritmo
	int sep_note_on; //determina se as freqs são ou não separadas por notas

	int taxaMaxNegativa; //índice do fcentral da freq que tem taxa de variação negativa mais negativa
	int taxaMinNegativa; //idem
	int taxaMaxPositiva; //idem    
	int taxaMinPositiva; //idem    

	int freqMaxAmp; //ínidice da frequência com amp máxima
	int freqMaxAmp2;
	int freqMinAmp; //idem

	int max_freq_to_algo; //valor de frequência a partir do qual não se considera para o algorítmo ou cálculos

	int tempo_fixo_cresce; //numero de frames que a nota fixa pelo algortimo no crescimento fica na amplitude maxima

	float taxadecresce; //limite de crescimento para entrar no algoritmo
	float taxacresce; //limite de diminuição para entrar no algorítmo

	float cresce_algo_mult; //factor multiplicador da taxa de variação quando uma nota entra no algortimo
	float decresce_algo_mult; //factor multiplicador da taxa de variação quando uma nota entra no algortimo

	float ratioMinDimMult; //ratio minimo para taxa de diminuição (multiplicador)
	float ratioMinDimDivid; //ratio minimo para taxa de diminuição (divsor)

	float ratioMinCrescMult; //ratio minimo para taxa de crescimento (multiplicador)
	float ratioMinCrescDivid; //ratio minimo para taxa de crescimento (divsor)

	float ratioMaxDimMult; //ratio maximo (do dinâmico) para taxa de diminuição (multiplicador)
	float ratioMaxDimDivid; //ratio maximo (do dinâmico) para taxa de diminuição (divsor)

	float ratioMaxCrescMult; //ratio maximo (do dinâmico) para taxa de crescimento (multiplicador)
	float ratioMaxCrescDivid; //ratio maximo (do dinâmico) para taxa de crescimento (divsor)

	int difSup;
	int difDim;
};

struct notas { //estrutura que define cada frequência

	int hz;
	float ddown;
	float dup;
	float ampR[100];
	int ampRindice;
	int osc;
	float slope;
	float ampE;

	unsigned int frame;
	unsigned int framesactive;

	int inalgo_cresce;
	int inalgo_decresce;

};

struct freqs { //estrutura que define cada frequência de entrada (lista do sigmund)

	int osc;
	int freq;
	float amp;

	int oscFund; //oscilador da fundamental de que é harmónico. -1 se for nota
	int multHarm; //0 se for fundamental. Caso seja hamrónico, igual ao multiplicador

};

typedef struct _fbcontrolreact { //estrutura que define o objecto

	t_object s_obj;

	void* m_outlet1;
	void* m_outlet;

	long m_in;
	void* m_proxy1;
	void* m_proxy;

	int size_freqs_in;
	int size_fcentral;
	int globalFrame; //Frame em que o programa está (conta  1 cada ciclo de osc=0 a osc=0)

	struct notas fcentral[FCENTRALSIZE];
	struct freqs freqs_in[FREQSINSIZE];

	struct notas* ptr_fcentral[FCENTRALSIZE];
	struct freqs* ptr_freqs_in[FREQSINSIZE];

	struct flags_total flags;

	t_atom list_out[4];
	t_atom amps_proibidas_out[2];

} t_fbcontrolreact;

static t_class* s_fbcontrolreact_class;

void* fbcontrolreact_new();
void fbcontrolreact_list(t_fbcontrolreact*, t_symbol*, long, t_atom*);

float note_accepted_deviation(int); //calcula o desvio aceitavel para reconhecer uma frequencia como outra
void quicksort_array_of_structures(t_fbcontrolreact*, int, int, int);//algoritmo de sorting. 0 se for do freqs_in, 1 se for do fcentral. Faz sort de array of pointers to structures
int check_partial(int, int, int);//compara duas frequênicas e um multiplicador, verificando se a hz2 é partial de hz1


void fbcontrolreact_init_flags(t_fbcontrolreact*); //função que inicia os valores da estrutura "flags" que determinam o comportamento do algorítmo
void fbcontrolreact_fill_freqs_in(t_fbcontrolreact*, int*, long, float, float, long); //adiciona ao freqs_in a informação da lista. Incrementa +1 à variável que conta o tamanho do freqs_in
int fbcontrolreact_in_alg(t_fbcontrolreact*, int, float); //para uma determinada nota do fcentral, verifica se esta deverá entrar para o algorítmo de controlo ou não
float fbcontrolreact_new_amp(t_fbcontrolreact*, int, float); //devolve a nova amplitude para uma determinada frequencia consoante o algortimo ativo
void fbcontrolreact_freqs_in_notes(t_fbcontrolreact*, int);//numa estrutura do tipo freqs (sorted), encontra as notas e harmónicos
void fbcontrolreact_update_fcentral(t_fbcontrolreact*, int, int*); //atualiza o fcentral, adicionado notas, organizando o pointer, eliminado residuos
void fbcontrolreact_dup_ddown_calc(t_fbcontrolreact*, int); //calcula os desvios para cada nota do fcentral
void fbcontrolreact_fcentral_inactive_note_delete(t_fbcontrolreact*, int*, int); //elimina notas inativas do fcentral
void fbcontrolreact_freqsin_addto_fcentral(t_fbcontrolreact*, int, int, int); //adiciona uma frequência do freqs_in ao fcentral
void fbcontrolreact_ampR_slope_calc(t_fbcontrolreact* x, int size_fcentral); //calcula declives com base nas amplitudes em ampR
int fbcontrolreact_compare_freqs_in(t_fbcontrolreact*, int, float, float, long, float*); //compara a frequencia que está a ser analisada com os valores em fcentral, procurando a sua existência 
void fbcontrolreact_flags_calc(t_fbcontrolreact*, int); //calcula as flags de ampMax, ampMin, taxas de variação, etc
float fbcontrolreact_calc_taxa(t_fbcontrolreact* ,float, int); //dá o valor de taxa de variação


void ext_main(void* r)
{

	t_class* c;
	c = class_new("fbcontrolreact~", (method)fbcontrolreact_new, (method)NULL, sizeof(t_fbcontrolreact), 0L, 0);

	class_addmethod(c, (method)fbcontrolreact_list, "list", A_GIMME, 0);

	class_register(CLASS_BOX, c);

	s_fbcontrolreact_class = c;

}

void* fbcontrolreact_new()
{
	t_fbcontrolreact* x = (t_fbcontrolreact*)object_alloc(s_fbcontrolreact_class);

	x->m_proxy1 = proxy_new((t_object*)x, 2, &x->m_in);
	x->m_proxy = proxy_new((t_object*)x, 1, &x->m_in);


	x->m_outlet1 = listout(x);
	x->m_outlet = listout(x);

	x->globalFrame = -1;
	x->size_freqs_in = 0;
	x->size_fcentral = 0;


	fbcontrolreact_init_flags(x);//inicializa as flags	
		
	return x;

}

void fbcontrolreact_list(t_fbcontrolreact* x, t_symbol* s, long argc, t_atom* argv)
{

	if (proxy_getinlet((t_object*)x) == 0) //lista recebida no primeiro inlet
	{

		int indice_note_fcentral;
		t_atom* list_values = argv; //pointer para o primeiro valor da lista

		//guardar os valores da lista (sempre na forma [osc,freq,amp,flag])
		long l_osc = atom_getlong(list_values);
		float l_hz = atom_getfloat(list_values + 1);
		float l_amp = atom_getfloat(list_values + 2);		
		long l_flag = atom_getlong(list_values + 3);

		float l_amp_new = l_amp; //variável que conterá, caso necessário, a nova amplitude após algoritmo

		//preparar dados para o outlet
		atom_setlong(x->list_out, l_osc);
		atom_setfloat(x->list_out + 1, l_hz);
		atom_setfloat(x->list_out + 2, l_amp);
		atom_setlong(x->list_out + 3, l_flag);


		if (x->globalFrame > -1)//estamos em análise ativa
		{

			if (l_osc != 0)
			{

				indice_note_fcentral = fbcontrolreact_compare_freqs_in(x, x->size_fcentral, l_hz, l_amp, l_flag, &l_amp);

				if ( indice_note_fcentral == -1) //não é nota, está a baixo do valor de amp min ou estamos no primeiro frame
				{	

					atom_setfloat(x->list_out + 2, l_amp);

					fbcontrolreact_fill_freqs_in(x, &(x->size_freqs_in), l_osc, l_hz, l_amp, l_flag); //preenche o freqs_in caso seja freq ativa

					outlet_list(x->m_outlet, NULL, 4, x->list_out);

				}
				else if(x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce >= 1 || x->ptr_fcentral[indice_note_fcentral]->inalgo_decresce == 1 || fbcontrolreact_in_alg(x, indice_note_fcentral, l_amp) == 1) //verifica se esta no algoritmo ou se deveria estar
				{
				
					l_amp_new = fbcontrolreact_new_amp(x, indice_note_fcentral, l_amp);

					atom_setfloat(x->list_out + 2, l_amp_new);
					fbcontrolreact_fill_freqs_in(x, &(x->size_freqs_in), l_osc, l_hz, l_amp, l_flag);

					outlet_list(x->m_outlet, NULL, 4, x->list_out);
						
				}
				else //é nota mas não está no algorítmo
				{
				
					fbcontrolreact_fill_freqs_in(x, &(x->size_freqs_in), l_osc, l_hz, l_amp, l_flag);
					x->ptr_fcentral[indice_note_fcentral]->ampE = l_amp;

					outlet_list(x->m_outlet, NULL, 4, x->list_out);

				}
				
			}
			else
			{
								
				quicksort_array_of_structures(x, 0, x->size_freqs_in - 1, 0);//sort freqs_in				
				fbcontrolreact_freqs_in_notes(x, x->size_freqs_in);				
				fbcontrolreact_update_fcentral(x, x->size_freqs_in, &x->size_fcentral);
				fbcontrolreact_ampR_slope_calc(x, x->size_fcentral);
				fbcontrolreact_flags_calc(x, x->size_fcentral);	
				
				x->size_freqs_in = 0;

				x->globalFrame += 1;
				indice_note_fcentral = fbcontrolreact_compare_freqs_in(x, x->size_fcentral, l_hz, l_amp, l_flag, &l_amp);

				if (indice_note_fcentral == -1) //não é nota ou estamos no primeiro frame
				{

					atom_setfloat(x->list_out + 2, l_amp);

					fbcontrolreact_fill_freqs_in(x, &(x->size_freqs_in), l_osc, l_hz, l_amp, l_flag); //preenche o freqs_in caso seja freq ativa

					outlet_list(x->m_outlet, NULL, 4, x->list_out);

				}
				else if (x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce >= 1 || x->ptr_fcentral[indice_note_fcentral]->inalgo_decresce == 1 || fbcontrolreact_in_alg(x, indice_note_fcentral, l_amp) == 1) //verifica se esta no algoritmo ou se deveria estar
				{
					
					l_amp_new = fbcontrolreact_new_amp(x, indice_note_fcentral, l_amp);
					atom_setfloat(x->list_out + 2, l_amp_new);
					fbcontrolreact_fill_freqs_in(x, &x->size_freqs_in, l_osc, l_hz, l_amp, l_flag);

					outlet_list(x->m_outlet, NULL, 4, x->list_out);

				}
				else
				{

					fbcontrolreact_fill_freqs_in(x, &x->size_freqs_in, l_osc, l_hz, l_amp, l_flag);
					x->ptr_fcentral[indice_note_fcentral]->ampE = l_amp;

					outlet_list(x->m_outlet, NULL, 4, x->list_out);

				}	

			}

		}
		else if(l_osc == 0) //primeiro osc = 0
		{

			x->globalFrame += 1;

			fbcontrolreact_fill_freqs_in(x, &x->size_freqs_in, l_osc, l_hz, l_amp, l_flag); //preenche o freqs_in caso seja freq ativa			

			outlet_list(x->m_outlet, NULL, 4, x->list_out);

		}
		else
		{

			outlet_list(x->m_outlet, NULL, 4, x->list_out);

		}

	}
	else if (proxy_getinlet((t_object*)x) == 1) //lista recebida no segundo inlet
	{

	t_atom* in_flag_change = argv;
	long in_flag = atom_getlong(in_flag_change);
	long in_value = atom_getlong(in_flag_change + 1);
	float in_valuef = atom_getfloat(in_flag_change + 1);
	static int marcador = 0;

	switch (in_flag)
	{
	case 1:

		if (marcador != 1) { post("Alterar a AmpMax"); }
		x->flags.ampmax = in_valuef;
		marcador = 1;

		break;

	case 2:

		if (marcador != 2) { post("Alterar a AmpMin"); }
		x->flags.ampmin = in_valuef;
		marcador = 2;

		break;

	case 3:

		if (marcador != 3) { post("Ligar/Desligar Algoritmo"); }
		x->flags.algoOn = in_value;
		marcador = 3;

		break;

	case 4:

		if (marcador != 4) { post("Alterar a Taxa de dimnuição para ativar algoritmo"); }
		x->flags.taxadecresce = in_valuef;

		marcador = 4;

		break;

	case 5:

		if (marcador != 5) { post("Alterar a Taxa de Crescimento para ativar algoritmo"); }
		x->flags.taxacresce = in_valuef;

		marcador = 5;

		break;
	case 6:

		if (marcador != 6) { post("Ligar/Desligar separacao por notas"); }
		x->flags.sep_note_on = in_value;

		marcador = 6;

		break;

	case 7:

		if (marcador != 7) { post("Frames para ativar uma nota nova"); }
		x->flags.frames_to_active = in_value;

		marcador = 7;

		break;

	case 8:

		if (marcador != 8) { post("Max freq a entrar no algortimo"); }
		x->flags.max_freq_to_algo = in_value;

		marcador = 8;

		break;

	case 9:

		if (marcador != 9) { post("Numero de ampsR a ler"); }
		x->flags.n_ampR_read = in_value;

		marcador = 9;

		break;

	case 10:

		if (marcador != 10) { post("Multiplicador de crescimento"); }
		x->flags.cresce_algo_mult = in_valuef;

		marcador = 10;

		break;

	case 11:

		if (marcador != 11) { post("Multiplicador de diminuicao"); }
		x->flags.decresce_algo_mult = in_valuef;

		marcador = 11;

		break;

	case 12:

		if (marcador != 12) { post("Tempo que fica fixa no crescimento"); }
		x->flags.tempo_fixo_cresce = in_value;

		marcador = 12;

		break;

	default:
		post("Essa flag não existe\n");
		break;
	}

	}
	else //lista recebida no terceiro inlet
	{

	t_atom* in_notas_proibidas = argv;
	long in_nota_1 = atom_getlong(in_notas_proibidas);
	long in_nota_2 = atom_getlong(in_notas_proibidas + 1);

	x->flags.nota_proibidaR = in_nota_1;
	x->flags.nota_proibidaR2 = in_nota_2;

	}
}

void fbcontrolreact_init_flags (t_fbcontrolreact* x)
{

	//inicializar as estruturas
	x->fcentral[0].hz = -1; //para que seja possível disntiguir a primeira vez que se acede ao fcentral
	x->freqs_in[0].freq = -1; //para que seja possível disntiguir a primeira vez que se acede ao freqs_in

	//inicializar as flags de controlo
	x->flags.frames_to_active = 10;
	x->flags.n_ampR_read = 3;

	x->flags.ampmax = 1.0;
	x->flags.ampmin = 0.001;

	x->flags.nota_proibidaE = -1;
	x->flags.nota_proibidaE2 = -1;

	x->flags.nota_proibidaR = -1;
	x->flags.nota_proibidaR2 = -1;

	x->flags.algoOn = 1;
	x->flags.sep_note_on = 1;

	x->flags.difSup = 0;
	x->flags.difDim = 0;

	x->flags.taxacresce = 0.3;
	x->flags.taxadecresce = -0.3;

	x->flags.cresce_algo_mult = 6;
	x->flags.decresce_algo_mult = 6;

	x->flags.tempo_fixo_cresce = 20;

	x->flags.taxaMaxNegativa = -1;
	x->flags.taxaMinNegativa = -1;
	x->flags.taxaMaxPositiva = -1;
	x->flags.taxaMinPositiva = -1;

	x->flags.freqMaxAmp = -1;
	x->flags.freqMaxAmp2 = -1;
	x->flags.freqMinAmp = -1;

	x->flags.max_freq_to_algo = 5000;

	x->flags.ratioMinDimMult = 1.0;
	x->flags.ratioMinDimDivid = 1.0;

	x->flags.ratioMinCrescMult = 1.0;
	x->flags.ratioMinCrescDivid = 1.0;

	x->flags.ratioMaxDimMult = 1.0;
	x->flags.ratioMaxDimDivid = 1.0;

	x->flags.ratioMaxCrescMult = 1.0;
	x->flags.ratioMaxCrescDivid = 1.0;

}

void fbcontrolreact_fill_freqs_in(t_fbcontrolreact* x, int* size_freqs_in, long osc, float hz, float amp, long flag) //adiciona ao freqs_in a informação da lista. Incrementa +1 à variável que conta o tamanho do freqs_in
{

	if (flag == 0 && hz < x->flags.max_freq_to_algo)
	{
		int size = *size_freqs_in;

		x->freqs_in[size].freq = hz;
		x->freqs_in[size].amp = amp;
		x->freqs_in[size].osc = osc;

		x->freqs_in[size].multHarm = -1;
		x->freqs_in[size].oscFund = -1;

		x->ptr_freqs_in[size] = &x->freqs_in[size];

		*size_freqs_in += 1;
		
	}
	

}


int fbcontrolreact_compare_freqs_in(t_fbcontrolreact* x, int size_fcentral, float hz, float amp, long flag, float* ptr_l_amp) //compara a frequencia que está a ser analisada com os valores em fcentral, procurando a sua existência 
{

	int i;

	float desvioMax, desvioMax2;

	if (hz > x->flags.max_freq_to_algo) { return -1; } //se a amplitude estiver a baixo do valor mínimo ou a freq for maior que o valor máximo, não é considerada

	desvioMax = (x->flags.nota_proibidaR / 12) / 2;
	desvioMax2 = (x->flags.nota_proibidaR2 / 12) / 2;

	if ((hz > x->flags.nota_proibidaR - desvioMax && hz < x->flags.nota_proibidaR + desvioMax) || (hz > x->flags.nota_proibidaR2 - desvioMax2 && hz < x->flags.nota_proibidaR2 + desvioMax2))
	{

		*ptr_l_amp = 0;

		return -1;

	}

	for (i = 0; i < size_fcentral; i++)
	{

		if (hz < x->ptr_fcentral[i]->ddown)
		{

			return -1;

		}
		else if (hz >= x->ptr_fcentral[i]->dup)
		{

			continue;

		}
		else //estamos dentro
		{
			if (abs(x->globalFrame - x->ptr_fcentral[i]->frame) > FRAMESTODELETE || (amp < x->flags.ampmin && x->ptr_fcentral[i]->inalgo_decresce != 1))
			{

				return -1;

			}
			else
			{

				return i;

			}
		}

	}

	return -1;

}

int fbcontrolreact_in_alg(t_fbcontrolreact* x, int indice_fcentral, float amp_actual) //para uma determinada nota do fcentral, verifica se esta deverá entrar para o algorítmo de controlo ou não
{

	if (x->flags.algoOn == 1 && x->ptr_fcentral[indice_fcentral]->framesactive > x->flags.frames_to_active) //se o algorítmo estive ativo e a frequencia já for considerada ativa
	{

		if (x->ptr_fcentral[indice_fcentral]->slope > x->flags.taxacresce && indice_fcentral == x->flags.freqMaxAmp)
		{
			
			x->ptr_fcentral[indice_fcentral]->inalgo_cresce = 1;

			return 1; //ativou o algortimo 

		}
		else if (x->ptr_fcentral[indice_fcentral]->slope < x->flags.taxadecresce)
		{

			x->ptr_fcentral[indice_fcentral]->inalgo_decresce = 1;

			return 1; //ativou o algortimo 

		}
	}

	return 0; //não ativou o algorítmo

}

float fbcontrolreact_new_amp(t_fbcontrolreact* x, int indice_note_fcentral, float amp) //devolve a nova amplitude para uma determinada frequencia consoante o algortimo ativo
{
	float nova_amp;
	float nova_taxa;

	if (x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce == 1)
	{

		nova_taxa = fbcontrolreact_calc_taxa(x, x->ptr_fcentral[indice_note_fcentral]->ampE, 1);
		nova_amp = (expf(nova_taxa * 0.1)) * x->ptr_fcentral[indice_note_fcentral]->ampE;

		if (nova_amp > x->flags.ampmax || x->ptr_fcentral[indice_note_fcentral]->slope < x->flags.taxacresce)
		{
			
			if (nova_amp > x->flags.ampmax)
			{
				nova_amp = x->flags.ampmax;

			}

			x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce += 1;

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;

			return nova_amp;			

		}
		else 
		{

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;
			return nova_amp;
		
		}

	}
	else if(x->ptr_fcentral[indice_note_fcentral]->inalgo_decresce == 1)
	{

		nova_taxa = fbcontrolreact_calc_taxa(x, x->ptr_fcentral[indice_note_fcentral]->ampE, 0);
		nova_amp = (expf(nova_taxa * 0.1)) * x->ptr_fcentral[indice_note_fcentral]->ampE;

		if (nova_amp < 0)
		{

			x->ptr_fcentral[indice_note_fcentral]->inalgo_decresce = 0;
			x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce = 0;

			nova_amp = 0;

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;

			return nova_amp;
			

		}
		else
		{

			if (nova_amp > x->flags.ampmax)
			{
				nova_amp = x->flags.ampmax;

			}

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;
			return nova_amp;

		}		

	}
	else if (x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce > 1)
	{

		if (x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce > x->flags.tempo_fixo_cresce)
		{

			x->ptr_fcentral[indice_note_fcentral]->inalgo_decresce = 0;
			x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce = 0;

			nova_amp = x->flags.ampmax;

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;

			return nova_amp;


		}
		else
		{


			nova_amp = x->flags.ampmax;

			x->ptr_fcentral[indice_note_fcentral]->inalgo_cresce += 1;

			x->ptr_fcentral[indice_note_fcentral]->ampE = nova_amp;

			return nova_amp;

		}

	}

}

void quicksort_array_of_structures(t_fbcontrolreact* x, int first, int last, int freqs_in_or_fcentral) //algoritmo de sorting. 0 se for do freqs_in, 1 se for do fcentral. Faz sort de array of pointers to structures
{

	int i, j, pivot;

	if (freqs_in_or_fcentral == 0)
	{
		struct freqs* temp;

		if (first < last) 
		{
			pivot = first;
			i = first;
			j = last;

			while (i < j) 
			{
				while (x->ptr_freqs_in[i]->freq <= x->ptr_freqs_in[pivot]->freq && i < last)
				{

					i++;

				}

				while (x->ptr_freqs_in[j]->freq > x->ptr_freqs_in[pivot]->freq)
				{
				
					j--;

				}

				if (i < j) 
				{
					temp = x->ptr_freqs_in[i];
					x->ptr_freqs_in[i] = x->ptr_freqs_in[j];
					x->ptr_freqs_in[j] = temp;
				}
			}

			temp = x->ptr_freqs_in[pivot];
			x->ptr_freqs_in[pivot] = x->ptr_freqs_in[j];
			x->ptr_freqs_in[j] = temp;
			quicksort_array_of_structures(x, first, j - 1, 0);
			quicksort_array_of_structures(x, j + 1, last, 0);

		}
	}
	else
	{

		struct notas* temp;

		if (first < last) 
		{
			pivot = first;
			i = first;
			j = last;

			while (i < j) {
				while (x->ptr_fcentral[i]->hz <= x->ptr_fcentral[pivot]->hz && i < last)
				{
					i++;
				}
					
				while (x->ptr_fcentral[j]->hz > x->ptr_fcentral[pivot]->hz)
				{
					j--;
				}
					
				if (i < j) 
				{
					temp = x->ptr_fcentral[i];
					x->ptr_fcentral[i] = x->ptr_fcentral[j];
					x->ptr_fcentral[j] = temp;
				}
			}

			temp = x->ptr_fcentral[pivot];
			x->ptr_fcentral[pivot] = x->ptr_fcentral[j];
			x->ptr_fcentral[j] = temp;
			quicksort_array_of_structures(x, first, j - 1, 1);
			quicksort_array_of_structures(x, j + 1, last, 1);

		}

	}

}

void fbcontrolreact_freqs_in_notes(t_fbcontrolreact* x, int size)//numa estrutura do tipo freqs (sorted), encontra as notas e harmónicos
{

	int i, n, c, partial, harm_found;

	if (x->flags.sep_note_on == 1)//queremos separar por notas
	{
		for (i = 0; i < size; i++)
		{

			if (x->ptr_freqs_in[i]->multHarm > 0) { continue; } //saltar ciclo caso freq já tenha sido definida como partial ou esteja a baixo da amplitude minima

			if (x->ptr_freqs_in[i]->amp < x->flags.ampmin)
			{
				
				x->ptr_freqs_in[i]->oscFund = -2; //-2 é simbolo para ruído, não entra no fcentral nem é considerado fundamental ou partial
				continue;

			}
			else
			{

				harm_found = 0; //numero de harmonicos encontrados para deeterminada fundamental
				n = 2; //multiplciador faz reset a 2 sempre que voltar
				c = i + 1; //c é a frequência com a qual compara, que no inicio é sempre a seguinte
				
				while (c < size && n - harm_found < 4)
				{
					
					if (x->ptr_freqs_in[c]->amp < x->flags.ampmin)
					{

						x->ptr_freqs_in[c]->oscFund = -2;//é ruido
						c += 1;

					}
					else
					{
						
						partial = check_partial(x->ptr_freqs_in[i]->freq, x->ptr_freqs_in[c]->freq, n);

						if (x->ptr_freqs_in[c]->multHarm > 0) 
						{

							c += 1;

						}
						else if (partial == 1)
						{

							c += 1;

						}
						else if (partial == -1)
						{

							n += 1;

						}
						else
						{

							x->ptr_freqs_in[c]->oscFund = x->ptr_freqs_in[i]->osc;
							x->ptr_freqs_in[c]->multHarm = n;

							harm_found += 1;
							c += 1;
							n += 1;

						}

					}

				}

			}
			
		}

	}
	else //consideramos tudo nota
	{

		for (i = 0; i < size; i++) {

			x->freqs_in[i].oscFund = -1;

		}

	}


}

float note_accepted_deviation(int hz)
{

	return ((float)hz / 12) / 2;

}

void fbcontrolreact_dup_ddown_calc(t_fbcontrolreact* x, int fcentral_size) //calcula os desvios para cada nota do fcentral
{

	int i = 0; //ciclo dos objectos
	float u = 0.0; //desvio positivo da Frequencia actual
	float d = 0.0; //desvio negativo da prox. frequÍncia

	for (i = 0; i < fcentral_size - 1; i++) {

		u = note_accepted_deviation(x->ptr_fcentral[i]->hz);
		d = note_accepted_deviation(x->ptr_fcentral[i + 1]->hz);

		if (x->ptr_fcentral[i]->hz + u < x->ptr_fcentral[i + 1]->hz - d) //intervalos n„o se sobrepoÍm
		{

			x->ptr_fcentral[i]->dup = x->ptr_fcentral[i]->hz + u;

			x->ptr_fcentral[i + 1]->ddown = x->ptr_fcentral[i + 1]->hz - d;

		}
		else //caso os intervalos coincidam
		{

			x->ptr_fcentral[i]->dup = (float)x->ptr_fcentral[i]->hz + ((float)x->ptr_fcentral[i + 1]->hz - (float)x->ptr_fcentral[i]->hz) / 2;

			x->ptr_fcentral[i + 1]->ddown = (float)x->ptr_fcentral[i + 1]->hz - ((float)x->ptr_fcentral[i + 1]->hz - (float)x->ptr_fcentral[i]->hz) / 2;

		}

	}

	if (fcentral_size > 0)  //definir o ddown da primeira e o dup da ˙ltima 
	{

		x->ptr_fcentral[0]->ddown = (float)x->ptr_fcentral[0]->hz - note_accepted_deviation(x->ptr_fcentral[0]->hz);

		x->ptr_fcentral[fcentral_size - 1]->dup = (float)x->ptr_fcentral[fcentral_size - 1]->hz + note_accepted_deviation(x->ptr_fcentral[fcentral_size - 1]->hz);

	}

}

void fbcontrolreact_fcentral_inactive_note_delete(t_fbcontrolreact* x, int* fcentral_size, int freqsin_size) //elimina notas do fim do fcentral que já estejam inativas
{	
	int i, k;
		
	if (*fcentral_size > 2 * freqsin_size) //se o fcentral se estiver a tornar demasiado grande, faz um ciclo de limpeza
	{

		k = 0;

		for (i = 0; i < *fcentral_size; i++)
		{
			if (abs(x->globalFrame - x->fcentral[i].frame) <= FRAMESTODELETE)
			{

				if (k == i)
				{
					x->ptr_fcentral[k] = &x->fcentral[k];
					k += 1;
				}
				else
				{
					x->fcentral[k] = x->fcentral[i];
					x->ptr_fcentral[k] = &x->fcentral[k];
					k += 1;

				}

			}
		}

		if (k == 0) 
		{ 
			k = 1;
			x->ptr_fcentral[0] = &x->fcentral[0];
		}
		*fcentral_size = k;

	}

}

void fbcontrolreact_freqsin_addto_fcentral(t_fbcontrolreact* x, int indice_freqsin, int indice_fcentral, int subs_or_add_or_end) //adiciona uma frequência do freqs_in ao fcentral
{
	if (subs_or_add_or_end == 1) //adicionar (freqs iguais)
	{

		if (abs(x->globalFrame - x->ptr_fcentral[indice_fcentral]->frame) > FRAMESTODELETE)//está marcada para apagar, substituir tudo
		{					

			x->ptr_fcentral[indice_fcentral]->ampRindice = 1;
			x->ptr_fcentral[indice_fcentral]->inalgo_cresce = 0;
			x->ptr_fcentral[indice_fcentral]->inalgo_decresce = 0;
			x->ptr_fcentral[indice_fcentral]->ddown = 0.0;
			x->ptr_fcentral[indice_fcentral]->dup = 0.0;

			x->ptr_fcentral[indice_fcentral]->frame = x->globalFrame;
			x->ptr_fcentral[indice_fcentral]->framesactive = 1;

			x->ptr_fcentral[indice_fcentral]->ampR[0] = x->ptr_freqs_in[indice_freqsin]->amp;
			x->ptr_fcentral[indice_fcentral]->ampR[1] = x->ptr_freqs_in[indice_freqsin]->amp;

			x->ptr_fcentral[indice_fcentral]->ampE = x->ptr_freqs_in[indice_freqsin]->amp;

			x->ptr_fcentral[indice_fcentral]->osc = x->ptr_freqs_in[indice_freqsin]->osc;
		}
		else//continuação
		{

			if (x->ptr_fcentral[indice_fcentral]->ampRindice >= 99) { x->ptr_fcentral[indice_fcentral]->ampRindice = -1; }//caso estejemos no ultimo indice do ampsR, começamos a colocar de inicio

			x->ptr_fcentral[indice_fcentral]->ampR[x->ptr_fcentral[indice_fcentral]->ampRindice + 1] = x->ptr_freqs_in[indice_freqsin]->amp;
			x->ptr_fcentral[indice_fcentral]->ampRindice += 1;

			x->ptr_fcentral[indice_fcentral]->frame = x->globalFrame;
			x->ptr_fcentral[indice_fcentral]->framesactive += 1;

			x->ptr_fcentral[indice_fcentral]->osc = x->ptr_freqs_in[indice_freqsin]->osc;

			if (x->ptr_fcentral[indice_fcentral]->framesactive < x->flags.frames_to_active) { x->ptr_fcentral[indice_fcentral]->ampE = x->ptr_freqs_in[indice_freqsin]->amp; }
		}

	}
	else if (subs_or_add_or_end == 0) //substituir (freqs diferentes)
	{

		x->ptr_fcentral[indice_fcentral]->hz = x->ptr_freqs_in[indice_freqsin]->freq;
		x->ptr_fcentral[indice_fcentral]->ddown = 0.0;
		x->ptr_fcentral[indice_fcentral]->dup = 0.0;
		x->ptr_fcentral[indice_fcentral]->osc = x->ptr_freqs_in[indice_freqsin]->osc;
		x->ptr_fcentral[indice_fcentral]->ampR[0] = x->ptr_freqs_in[indice_freqsin]->amp;
		x->ptr_fcentral[indice_fcentral]->ampR[1] = x->ptr_freqs_in[indice_freqsin]->amp;

		x->ptr_fcentral[indice_fcentral]->ampE = x->ptr_freqs_in[indice_freqsin]->amp;

		x->ptr_fcentral[indice_fcentral]->ampRindice = 1;

		x->ptr_fcentral[indice_fcentral]->framesactive = 1;

		x->ptr_fcentral[indice_fcentral]->frame = x->globalFrame;

		x->ptr_fcentral[indice_fcentral]->inalgo_cresce = 0;
		x->ptr_fcentral[indice_fcentral]->inalgo_decresce = 0;
		
	}
	else //adicionar no fim
	{

		x->fcentral[indice_fcentral].hz = x->ptr_freqs_in[indice_freqsin]->freq;
		x->fcentral[indice_fcentral].ddown = 0.0;
		x->fcentral[indice_fcentral].dup = 0.0;
		x->fcentral[indice_fcentral].osc = x->ptr_freqs_in[indice_freqsin]->osc;
		x->fcentral[indice_fcentral].ampR[0] = x->ptr_freqs_in[indice_freqsin]->amp;
		x->fcentral[indice_fcentral].ampR[1] = x->ptr_freqs_in[indice_freqsin]->amp;

		x->fcentral[indice_fcentral].ampE = x->ptr_freqs_in[indice_freqsin]->amp;

		x->fcentral[indice_fcentral].ampRindice = 1;

		x->fcentral[indice_fcentral].framesactive = 1;

		x->fcentral[indice_fcentral].frame = x->globalFrame;

		x->fcentral[indice_fcentral].inalgo_cresce = 0;
		x->fcentral[indice_fcentral].inalgo_decresce = 0;

		x->ptr_fcentral[indice_fcentral] = &x->fcentral[indice_fcentral];

	}

}

void fbcontrolreact_update_fcentral(t_fbcontrolreact* x, int freqsin_size, int* fcentral_size) //atualiza o fcentral, adicionado notas, organizando o pointer, eliminado residuos
{

	int i, k, k_anterior, hz_anterior;
	int after_fcentral;

	hz_anterior = 0;

	if (x->fcentral[0].hz == -1) //verificar se é a primeira vez que chamamos esta função (fcentral vazio)
	{
		k = 0;

		for (i = 0; i < freqsin_size; i++) {

			if (x->ptr_freqs_in[i]->oscFund == -1 && hz_anterior != x->ptr_freqs_in[i]->freq) //apenas insere no fcentral se for nota e for diferente da nota anterior
			{

				x->fcentral[k].hz = x->ptr_freqs_in[i]->freq;
				x->fcentral[k].ddown = 0.0;
				x->fcentral[k].dup = 0.0;
				x->fcentral[k].osc = x->ptr_freqs_in[i]->osc;
				x->fcentral[k].ampR[0] = x->ptr_freqs_in[i]->amp;
				x->fcentral[k].ampR[1] = x->ptr_freqs_in[i]->amp;

				x->fcentral[k].ampE = x->ptr_freqs_in[i]->amp;

				x->fcentral[k].ampRindice = 1;

				x->fcentral[k].framesactive = 1;

				x->fcentral[k].frame = x->globalFrame;

				x->fcentral[k].inalgo_cresce = 0;
				x->fcentral[k].inalgo_decresce = 0;

				x->ptr_fcentral[k] = &x->fcentral[k];

				hz_anterior = x->ptr_freqs_in[i]->freq;

				k += 1;
			}

		}

		*fcentral_size = k;

		fbcontrolreact_dup_ddown_calc(x, *fcentral_size);
		
	}
	else //fcentral já tem frequências
	{

		after_fcentral = 0;

		k_anterior = 0;
		hz_anterior = -1;

		k = 0;

		for (i = 0; i < freqsin_size; i++)
		{
				
			if (x->ptr_freqs_in[i]->oscFund == -1)//só vai adicionar ao fcentral se for nota
			{

				for (k = k_anterior; k < *fcentral_size; k++)
				{

					if (x->ptr_freqs_in[i]->freq < x->ptr_fcentral[k]->ddown)//ultrapssou o limite
					{

						if (hz_anterior != -1)
						{

							if (x->ptr_freqs_in[i]->freq == hz_anterior) //anterior e actual são iguais, ignorar a atual
							{

								break;

							}
							else
							{

								fbcontrolreact_freqsin_addto_fcentral(x, i, *fcentral_size + after_fcentral, 2);
								after_fcentral += 1;

								break;

							}

						}
						else
						{

							fbcontrolreact_freqsin_addto_fcentral(x, i, *fcentral_size + after_fcentral, 2);
							after_fcentral += 1;
							break;

						}

					}
					else if (x->ptr_freqs_in[i]->freq >= x->ptr_fcentral[k]->dup)//estou a cima, tenho de ir para a prox freq do fcentral
					{

						if (k >= *fcentral_size - 1) //estmaos no ultimo do fcentral, já não há nada a subir, terá de se adicionar no fim
						{

							if (hz_anterior != -1)
							{

								if (x->ptr_freqs_in[i]->freq == hz_anterior) //anterior e actual são iguais, ignorar a atual
								{

									break;

								}
								else
								{

									fbcontrolreact_freqsin_addto_fcentral(x, i, *fcentral_size + after_fcentral, 2);
									after_fcentral += 1;

									break;

								}

							}
							else
							{

								fbcontrolreact_freqsin_addto_fcentral(x, i, *fcentral_size + after_fcentral, 2);
								after_fcentral += 1;
								break;

							}

						}
						else
						{

							continue;

						}

					}
					else //estou dentro do intervalo desta freq do fcentral
					{

						if (hz_anterior != -1)
						{						

							if (x->ptr_freqs_in[i]->freq == hz_anterior) //anterior e actual são iguais, ignorar a atual
							{

								break;

							}
							else if (hz_anterior >= x->ptr_fcentral[k]->ddown && hz_anterior < x->ptr_fcentral[k]->dup)//freq anterior pertence ao intervalo da freq atual
							{

								fbcontrolreact_freqsin_addto_fcentral(x, i, *fcentral_size + after_fcentral, 2);
								after_fcentral += 1;
								
								break;

							}
							else
							{

								fbcontrolreact_freqsin_addto_fcentral(x, i, k, 1);
								break;
							
							}
						
						}
						else
						{
							
							fbcontrolreact_freqsin_addto_fcentral(x, i, k, 1);
							break;

						}

					}

				}

				k_anterior = k;
				hz_anterior = x->ptr_freqs_in[i]->freq;

			}
			
		}

		*fcentral_size += after_fcentral;

		fbcontrolreact_fcentral_inactive_note_delete(x, fcentral_size, freqsin_size);		

		quicksort_array_of_structures(x, 0, *fcentral_size - 1, 1);

		fbcontrolreact_dup_ddown_calc(x, *fcentral_size);
	}

}

void fbcontrolreact_ampR_slope_calc(t_fbcontrolreact* x, int size_fcentral) //calcula declives com base nas amplitudes em ampR
{
	int i, j, frames_to_read, indice;

	float zy; //somatÛrio de tempo*amp
	int z; //sumatÛrio de tempo
	float y; //sumatÛrio de amp
	int zz; //sumat·rio de (tempo)^2

	float s; //slope
	
	for (i = 0; i < size_fcentral; i++)
	{

		if (x->fcentral[i].framesactive <= x->flags.frames_to_active || abs(x->globalFrame - x->fcentral[i].frame) > FRAMESTODELETE)
		{

			continue;
		}
		else
		{

			if (x->fcentral[i].framesactive < x->flags.n_ampR_read) //esta ativa há menos frames do que o total de frames para o calculo do desvio
			{

				frames_to_read = x->fcentral[i].framesactive + 1;

			}
			else //podemos calcular com o ttoal de valores
			{

				frames_to_read = x->flags.n_ampR_read;

			}

			zy = 0;
			z = 0;
			y = 0;
			zz = 0;

			indice = x->fcentral[i].ampRindice;

			for (j = 0; j < frames_to_read; j++)
			{

				if (indice == -1)
				{
					indice = 99;
				}

				//calculo a fazer
				zy = zy + (frames_to_read - j) * logf(x->fcentral[i].ampR[indice]);
				z = z + (frames_to_read - j);
				y = y + logf(x->fcentral[i].ampR[indice]);
				zz = zz + (frames_to_read - j) * (frames_to_read - j);

				indice -= 1;
			}

			s = ((frames_to_read * zy - z * y) / (frames_to_read * zz - z * z)) / 0.1;

			x->fcentral[i].slope = s;

		}
	}

}

void fbcontrolreact_flags_calc(t_fbcontrolreact* x, int size_fcentral) //calcula as flags de ampMax, ampMin, taxas de variação, etc
{
	int i = 0; //fará o ciclo através do fcentral para verificar as taxas
	//var para atualizar as amplitudes max e min
	float ampMin = 100;
	int ampMinI = -1;
	float ampMax = -1;
	float ampMax2 = -1;
	int ampMaxI = -1;
	int ampMaxI2 = -1;
	//-----//------//
	//var para atualizar as taxas de variação max e mins (positivas e negativas)
	float taxaMinPos = 100;
	int taxaMinPosI = -1;
	float taxaMaxPos = -1;
	int taxaMaxPosI = -1;
	float taxaMinNeg = -100;
	int taxaMinNegI = -1;
	float taxaMaxNeg = 1;
	int taxaMaxNegI = -1;

	float slope;

	for (i = 0; i < size_fcentral; i++)
	{

		if (x->ptr_fcentral[i]->framesactive <= x->flags.frames_to_active || abs(x->globalFrame - x->ptr_fcentral[i]->frame) > FRAMESTODELETE) //apenas atualiza valores de frequências que foram ativadas neste frame	
		{

			continue;
		}
		else
		{		
			slope = x->ptr_fcentral[i]->slope;

			if (x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice] >= ampMax) //atualizar a amp max caso seja necesário
			{

				ampMax2 = ampMax;
				ampMax = x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice];
				ampMaxI2 = ampMaxI;
				ampMaxI = i;

			}
			else if (x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice] >= ampMax2)
			{

				ampMax2 = x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice];
				ampMaxI2 = i;

			}

			if (x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice] < ampMin)
			{

				ampMin = x->ptr_fcentral[i]->ampR[x->ptr_fcentral[i]->ampRindice];
				ampMinI = i;

			}

			if (slope > 0.001) //taxa min e max positiva
			{

				if (slope > taxaMaxPos) {

					taxaMaxPos = slope;
					taxaMaxPosI = i;

				}

				if (slope < taxaMinPos) {

					taxaMinPos = slope;
					taxaMinPosI = i;

				}

			}
			else if (slope < -0.001)  //taxa min e max negativa
			{

				if (slope > taxaMinNeg)
				{

					taxaMinNeg = slope;
					taxaMinNegI = i;

				}

				if (slope < taxaMaxNeg)
				{

					taxaMaxNeg = slope;
					taxaMaxNegI = i;

				}

			}

		}
	}

	x->flags.taxaMaxNegativa = taxaMaxNegI; //índice do fcentral da freq que tem taxa de variação negativa mais negativa
	x->flags.taxaMinNegativa = taxaMinNegI; //idem
	x->flags.taxaMaxPositiva = taxaMaxPosI; //idem    
	x->flags.taxaMinPositiva = taxaMinPosI; //idem    

	x->flags.freqMaxAmp = ampMaxI; //ínidice da frequência com amp máxima
	x->flags.freqMaxAmp2 = ampMaxI2;

	if (ampMaxI != -1) 
	{ 
		x->flags.nota_proibidaE = x->ptr_fcentral[ampMaxI]->hz; 
	}
	else
	{

		x->flags.nota_proibidaE = -1;

	}

	if (ampMaxI2 != -1) 
	{ 
	
		x->flags.nota_proibidaE2 = x->ptr_fcentral[ampMaxI2]->hz; 
	
	}
	else
	{

		x->flags.nota_proibidaE2 = -1;

	}

	atom_setlong(x->amps_proibidas_out, x->flags.nota_proibidaE);
	atom_setlong(x->amps_proibidas_out + 1, x->flags.nota_proibidaE2);

	outlet_list(x->m_outlet1, NULL, 2, x->amps_proibidas_out);



	x->flags.freqMinAmp = ampMinI; //idem

}

float fbcontrolreact_calc_taxa(t_fbcontrolreact* x, float amp, int cresce_descresce)
{

	if (cresce_descresce == 0)//esta a diminuir
	{

		return -x->flags.decresce_algo_mult;

	}
	else
	{
		//0.5272 * log(amp) + x->flags.decresce_algo_mult
		return x->flags.cresce_algo_mult;

	}

}

int check_partial(int hz1, int hz2, int mult)//compara duas frequênicas e um multiplicador, verificando se a hz2 é partial de hz1
{

	float y;
	y = (hz1 * mult) / 60;

	if (hz1 * mult < hz2 + y && hz1 * mult > hz2 - y)  //é harmónico
	{

		return 0;

	}
	else if (hz1 * mult < hz2 - y)  //é menor que a freq
	{

		return -1;

	}
	else  //é maior que a freq
	{

		return 1;

	}

}
